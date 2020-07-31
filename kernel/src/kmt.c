//
// Created by kruskallin on 13/6/2020.
//
#include <common.h>

#define MAX_CPU_NUM 8
#define INT_MIN 0
#define INT_MAX 255

static struct CPU_LOCAL cpus[MAX_CPU_NUM];
static task_t *last[MAX_CPU_NUM];
static task_t *current[MAX_CPU_NUM];
static task_t *idles[MAX_CPU_NUM];
static task_t *tasks[MAX_TASK_NUM];
static unsigned int tasks_len;

static pthread_mutex ktask;
static pthread_mutex kspin;
static pthread_mutex kspin_lock_lock;
static pthread_mutex kspin_unlock_lock;
static pthread_mutex ksem;


void pushcli() {
    _intr_write(0);
    int cpu_id = _cpu();
    if (cpus[cpu_id].ncli == 0)
        cpus[cpu_id].intena = _intr_read();
    cpus[cpu_id].ncli += 1;
}

void popcli() {
    panic_on(_intr_read(), "popcli - interruptible");
    int cpu_id = _cpu();
    panic_on(--cpus[cpu_id].ncli < 0, "popcli");
    if (!cpus[cpu_id].ncli && !cpus[cpu_id].intena) {
        cpus[cpu_id].intena = 1;
        _intr_write(1);
    }
}

static int holding(spinlock_t *lk) {
    int r;
    pushcli();
    r = lk->lock && lk->cpu == _cpu();
    popcli();
    return r;
}

static void kspin_init(spinlock_t *lk, const char *name) {
    pthread_mutex_lock(&kspin);
    memset(lk->name, '\0', sizeof(lk->name));
    strcpy(lk->name, name);
    lk->lock = 0;
    lk->cpu = -1;
    pthread_mutex_unlock(&kspin);
}

static void kspin_lock(spinlock_t *lk) {
    pushcli();
    if(holding(lk)) {
        char *info = pmm->alloc(sizeof(MAX_CHAR_LEN));
        strcpy(info, lk->name);
        strcat(info, " acquire");
        panic(info);
    }
    pthread_mutex_lock(&lk->lock);
    lk->cpu = _cpu();
}

static void kspin_unlock(spinlock_t *lk) {
    if(!holding(lk)) {
        char *info = pmm->alloc(sizeof(MAX_CHAR_LEN));
        strcpy(info, lk->name);
        strcat(info, " release");
        panic(info);
    }
    lk->cpu = -1;
    pthread_mutex_unlock(&lk->lock);
    popcli();
}

static void ksem_init(sem_t *sem, const char *name, int value) {
    pthread_mutex_lock(&ksem);
    memset(sem->name, '\0', sizeof(sem->name));
    strcpy(sem->name, name);
    sem->value = value;
    sem->waiting_tasks_len = 0;
    sem->lock = pmm->alloc(sizeof(spinlock_t));
    kspin_init(sem->lock, name);
    sem->waiting_tasks = pmm->alloc(MAX_TASK_NUM * sizeof(task_t * ));
    memset(sem->waiting_tasks, '\0', sizeof(MAX_TASK_NUM * sizeof(task_t * )));
    pthread_mutex_unlock(&ksem);
}

static void ksem_wait(sem_t *sem) {
    kspin_lock(sem->lock);
    sem->value--;
    if (sem->value < 0) {
        int cpu_id = _cpu();
        if (current[cpu_id]) {
            sem->waiting_tasks[sem->waiting_tasks_len++] = current[cpu_id];
            current[cpu_id]->block = 1;
        }
    }
    kspin_unlock(sem->lock);
    if (sem->value < 0) {
        _yield();
    }
}

static void ksem_signal(sem_t *sem) {
    kspin_lock(sem->lock);
    sem->value++;
    if (sem->value <= 0 && sem->waiting_tasks_len > 0) {
        int r = rand() % sem->waiting_tasks_len;
        sem->waiting_tasks[r]->block = 0;
        for (int i = r; i < sem->waiting_tasks_len - 1; i++)
            sem->waiting_tasks[i] = sem->waiting_tasks[i + 1];
        sem->waiting_tasks_len--;
    }
    kspin_unlock(sem->lock);
}

static void idle(void *arg) {
    while (1);
}

static int kcreate(task_t *task, const char *name, void (*entry)(void *), void *arg) {
    pthread_mutex_lock(&ktask);
    task->fence1 = 123456;
    task->fence2 = 654321;
    memset(task->name, '\0', sizeof(task->name));
    strcpy(task->name, name);
    memset(task->stack, '\0', sizeof(task->stack));
    task->context = _kcontext((_Area) {(void *) task->stack, (void *) (task->stack + STACK_SZ)}, entry, arg);
    task->state = 0;
    task->read_write = 0;
    task->count = 0;
    task->id = tasks_len;
    tasks[tasks_len++] = task;
    pthread_mutex_unlock(&ktask);
    return 0;
}

static void kteardown(task_t *task) {
    pthread_mutex_lock(&ktask);
    pmm->free(task);
    tasks_len--;
    pthread_mutex_unlock(&ktask);
}

static _Context *kmt_context_save(_Event ev, _Context *c) {
    int cpu_id = _cpu();
    if (!current[cpu_id]) {
        current[cpu_id] = idles[cpu_id];
    }
    current[cpu_id]->context = c;
/**
 * Unlock this state after the interruption is down, otherwise, the stack would be corrupted.
 * One way is unlock the state in the second interruption.
 */
    if(last[cpu_id] && last[cpu_id] != current[cpu_id])
        pthread_mutex_unlock(&last[cpu_id]->state);
    last[cpu_id] = current[cpu_id];
    panic_on(current[cpu_id]->fence1 != 123456 || current[cpu_id]->fence2 != 654321, "stackoverflow");
    return NULL;
}

static _Context *kmt_schedule(_Event ev, _Context *c) {
    int cpu_id = _cpu();
    task_t *task_interrupt = idles[cpu_id];
    unsigned int i = -1;
    for (int _ = 0; _ < tasks_len * 10; _++) {
        i = rand() % tasks_len;
        if (!tasks[i]->block && (tasks[i] == current[cpu_id] || !pthread_mutex_trylock(&tasks[i]->state))) {
            task_interrupt = tasks[i];
            break;
        }
    }
    current[cpu_id] = task_interrupt;
    panic_on(task_interrupt->fence1 != 123456 || task_interrupt->fence2 != 654321, "stackoverflow");
    return current[cpu_id]->context;
}


static void kmt_init() {
    os->on_irq(INT_MIN, _EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, _EVENT_NULL, kmt_schedule);

    tasks_len = 0;
    ktask = 0;
    kspin = 0;
    kspin_lock_lock = 0;
    kspin_unlock_lock = 0;
    ksem = 0;
    for (int i = 0; i < _ncpu(); i++) {
        idles[i] = pmm->alloc(4096);
        idles[i]->id = -1;
        memset(idles[i]->name, '\0', sizeof(idles[i]->name));
        strcpy(idles[i]->name, "IDLE");
        memset(idles[i]->stack, '\0', sizeof(idles[i]->stack));
        idles[i]->context = _kcontext((_Area) {(void *) idles[i]->stack, (void *) (idles[i]->stack + STACK_SZ)}, idle, NULL);
        idles[i]->state = 0;
        idles[i]->fence1 = 123456;
        idles[i]->fence2 = 654321;
        current[i] = NULL;
        last[i] = NULL;
        cpus[i].ncli = 0;
        cpus[i].intena = 1;
    }
}

MODULE_DEF(kmt) = {
        .init  = kmt_init,
        .create = kcreate,
        .teardown  = kteardown,
        .spin_init = kspin_init,
        .spin_lock = kspin_lock,
        .spin_unlock = kspin_unlock,
        .sem_init = ksem_init,
        .sem_wait = ksem_wait,
        .sem_signal = ksem_signal
};

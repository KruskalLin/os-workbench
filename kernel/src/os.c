//#define DEBUG
//#define DEBUG_PV
//#define DEBUG_NORMAL
//#define DEBUG_DEV

#ifdef DEBUG_DEV
 #include <devices.h>
#else
 #include <common.h>
#endif

extern char etext;

typedef struct irq {
    int seq;
    int event;
    handler_t handler;
    struct irq *next;
} irq;

static irq *irq_head = NULL;

#ifdef DEBUG_PV
static sem_t *empty, *fill;
#define P kmt->sem_wait
#define V kmt->sem_signal

void producer(void *arg) {
    while (1) {
        P(empty);
        _putc('(');
        V(fill);
    }
}

void consumer(void *arg) {
    while (1) {
        P(fill);
        _putc(')');
        V(empty);
    }
}

#endif

#ifdef DEBUG_NORMAL
static spinlock_t *idlelock;
static char *idles_name[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L"};
static void mock_task(void *arg) {
    while (1) {
        kmt->spin_lock(idlelock);
        printf("%s", arg);
        kmt->spin_unlock(idlelock);
        _yield();
//        for (int volatile i = 0; i < 100000; i++);
    }
}
#endif

#ifdef DEBUG_DEV
static void tty_reader(void *arg) {
    device_t *tty = dev->lookup(arg);
    char cmd[128], resp[128], ps[16];
    snprintf(ps, 16, "(%s) $ ", arg);
    while (1) {
        tty->ops->write(tty, 0, ps, strlen(ps));
        int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
        cmd[nread] = '\0';
        sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
        tty->ops->write(tty, 0, resp, strlen(resp));
    }
}
#endif

static void os_init() {
    pmm->init();
    kmt->init();
#ifdef DEBUG_NORMAL
    idlelock = pmm->alloc(sizeof(spinlock_t));
    kmt->spin_init(idlelock, "idlelock");
    for(int i = 0; i < 12; i++) {
        kmt->create(pmm->alloc(sizeof(task_t)), idles_name[i], mock_task, idles_name[i]);
    }
#endif
#ifdef DEBUG_PV
    empty = pmm->alloc(sizeof(sem_t));
    fill = pmm->alloc(sizeof(sem_t));
    kmt->sem_init(empty, "empty", 6);
    kmt->sem_init(fill, "fill", 0);
    for (int i = 0; i < 4; i++)
        kmt->create(pmm->alloc(sizeof(task_t)), "producer", producer, NULL);

    for (int i = 0; i < 5; i++)
        kmt->create(pmm->alloc(sizeof(task_t)), "consumer", consumer, NULL);
#endif
#ifdef DEBUG_DEV
    dev->init();
    kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty1");
    kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty2");
#endif

}

static void os_run() {
    _intr_write(1);
    while (1);
}

//static int sane_context(_Context *ctx) {
//    if(ctx->rip < (uint64_t)&etext && 0x100000 <= ctx->rip)
//        return 0;
//    return 1;
//}

static _Context *os_trap(_Event ev, _Context *ctx) {
    _Context *next = NULL;
    irq *current_irq = irq_head;
    while (current_irq != NULL) {
        if (current_irq->event == _EVENT_NULL || current_irq->event == ev.event) {
            _Context *r = current_irq->handler(ev, ctx);
            panic_on(r && next, "returning multiple contexts");
            if (r) next = r;
        }
        current_irq = current_irq->next;
    }
    panic_on(!next, "returning NULL context");
//    panic_on(sane_context(next), "returning to invalid context");
    return next;
}

static void os_on_irq(int seq, int event, handler_t handler) {
    irq *new_irq = pmm->alloc(sizeof(irq));
    new_irq->seq = seq;
    new_irq->event = event;
    new_irq->handler = handler;
    if (irq_head == NULL) {
        irq_head = new_irq;
    } else {
        if (irq_head->seq >= seq) {
            new_irq->next = irq_head;
            irq_head = new_irq;
        } else {
            irq *current_irq = irq_head;
            while (current_irq->next != NULL) {
                if (current_irq->next->seq > seq) {
                    new_irq->next = current_irq->next;
                    current_irq->next = new_irq;
                    return;
                }
                current_irq = current_irq->next;
            }
            current_irq->next = new_irq;
        }
    }
}

MODULE_DEF(os) = {
        .init = os_init,
        .run  = os_run,
        .trap   = os_trap,
        .on_irq = os_on_irq,
};

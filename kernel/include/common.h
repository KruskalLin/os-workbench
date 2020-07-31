#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

#define MAX_CHAR_LEN 128
#define MAX_TASK_NUM 16
#define STACK_SZ 1024

typedef volatile uintptr_t pthread_mutex;

struct CPU_LOCAL{
    int ncli;
    int intena;
};

typedef struct task{
    unsigned int id;
    char name[MAX_CHAR_LEN];
    _Context* context;
    uint32_t fence1;
    uint8_t stack[STACK_SZ];
    uint32_t fence2;
    int count;
    pthread_mutex read_write;
    pthread_mutex state;
    pthread_mutex block;
}task_t;

typedef struct spinlock{
    char name[MAX_CHAR_LEN];
    int cpu;
    pthread_mutex lock;
}spinlock_t;

typedef struct semaphore{
    char name[MAX_CHAR_LEN];
    volatile int value;
    volatile int waiting_tasks_len;
    struct spinlock *lock;
    volatile task_t** waiting_tasks;
}sem_t;

inline intptr_t atomic_xchg(volatile pthread_mutex *addr, intptr_t newval) {
    intptr_t result;
    asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "cc");
    return result;
}

inline void pthread_mutex_lock(pthread_mutex *lock) {
    while (atomic_xchg(lock, 1));
}

inline void pthread_mutex_unlock(pthread_mutex *lock) {
    asm volatile("movl $0, %0" : "+m" (*lock) : );
}

inline int pthread_mutex_trylock(pthread_mutex *lock) {
    return atomic_xchg(lock, 1);
}

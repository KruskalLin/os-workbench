#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ucontext.h>
#include <string.h>

#define STACK_SIZE 64 * 1024 // less than 64KBi
#define NAME_SIZE 256
#define COARRAY_SIZE 128

int current_id = -1;
int co_num = 0;
ucontext_t main_context;

struct co* co_array[COARRAY_SIZE];

enum co_status {
    CO_RUNNABLE,
    CO_DEAD
};

struct co {
    int id;

    char name[NAME_SIZE];
    void (*func)(void *);
    void *arg;

    enum co_status status;
    ucontext_t context;

    uint8_t stack[STACK_SIZE];

    struct co* next;
};

void exec_func(struct co *co) {
    co->func(co->arg);
    co->status = CO_DEAD;
    current_id = -1;
}


struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *co = malloc(sizeof(struct co));
    strcpy(co->name, name);
    co->id = co_num;
    co->func = func;
    co->arg = arg;
    co->status = CO_RUNNABLE;
    co->next = NULL;
    co_array[co_num++] = co;
    getcontext(&(co->context));
    memset(co->stack, 0, sizeof(co->stack));
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = sizeof(co->stack);
    co->context.uc_stack.ss_flags = 0;
    co->context.uc_link = &main_context;
    makecontext(&(co->context), (void (*)(void))exec_func, 1, co);
    return co;
}


void remove_co(struct co *co) {
    if (co == NULL)
        return;
    int index = -1;
    for(int i = 0; i < co_num; i++) {
        if(co_array[i] == co){
            index = i;
            break;
        }
    }
    for(int i = index; i < co_num - 1; i++) {
        co_array[i] = co_array[i + 1];
        co_array[i]->id = i;
    }
    free(co);
    co_num--;
}

void co_wait(struct co *co) {
    if (co->status == CO_DEAD) {
        remove_co(co);
    } else {
        while(co->status != CO_DEAD) {
            current_id = co->id;
            getcontext(&(main_context));
            swapcontext(&(main_context), &(co->context));
        }
        remove_co(co);
    }
}

//struct co* get(int times) {
//    struct co* next = head;
//    while(times > 0) {
//        next = next->next;
//        times--;
//    }
//    return next;
//}

void co_yield() {
    if(current_id == -1) {
        return;
    }

    int r = rand() % co_num;
    struct co *next = co_array[r];
    while(next->status == CO_DEAD) {
        r = rand() % co_num;
        next = co_array[r];
    }

    if(next->id == current_id)
        return;

    struct co* ori = co_array[current_id];
    current_id = next->id;
    next->context.uc_link = &(ori->context);
    swapcontext(&(ori->context), &(next->context));
}
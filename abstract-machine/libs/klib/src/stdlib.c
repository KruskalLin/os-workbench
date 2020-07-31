//
// Created by Popping Lim on 2020/3/14.
//

#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

typedef struct memory_control_block {
    size_t block_size;
    // merge prior block
    struct memory_control_block* prior_mcb;
    int available;
} MCB;


static unsigned long int next = 1;

int abs(int x) {
    return x >= 0 ? x : -x;
}

int rand(void) {
    // RAND_MAX assumed to be 32767
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) {
    next = seed;
}


// TODO
int atoi(const char* nptr){
    return 0;
}
unsigned long time() {
    return -1;
}

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {}

extern _Area _heap;

static int init = 0;

void malloc_init() {
    init = 1;
    MCB *mcb = (MCB *) _heap.start;
    mcb->block_size = _heap.end - _heap.start - sizeof(MCB);
    mcb->prior_mcb = NULL;
    mcb->available = 1;
}

void *malloc(size_t numbytes) {

    if (!init) {
        malloc_init();
    }

    void *current_loc;
    void *next_loc;
    void *next_next_loc;
    MCB *current_loc_mcb;
    MCB *next_loc_mcb;
    MCB *next_next_loc_mcb;

    void *malloc_loc = NULL;

    current_loc = _heap.start;

    while (current_loc != _heap.end) {
        current_loc_mcb = (MCB *) current_loc;
        /* check if current block is available. */
        if (current_loc_mcb->available && numbytes + sizeof(MCB) <= current_loc_mcb->block_size) {
            next_next_loc = current_loc + sizeof(MCB) + current_loc_mcb->block_size;
            current_loc_mcb->available = 0;
            current_loc_mcb->block_size = numbytes;
            next_loc = current_loc + sizeof(MCB) + numbytes;
            next_loc_mcb = (MCB *) next_loc;
            next_loc_mcb->available = 1;
            next_loc_mcb->block_size = next_next_loc - next_loc - sizeof(MCB);
            next_loc_mcb->prior_mcb = current_loc_mcb;
            if (next_next_loc != _heap.end) {
                next_next_loc_mcb = (MCB *) next_next_loc;
                next_next_loc_mcb->prior_mcb = next_loc_mcb;
            }
            malloc_loc = current_loc + sizeof(MCB);
            break;
        }
        current_loc = current_loc + sizeof(MCB) + current_loc_mcb->block_size;
    }

    return malloc_loc;
}

void free(void *firstbyte) {
    void *current_loc;
    void *next_loc;
    void *next_next_loc;
    MCB *current_loc_mcb;
    MCB *next_loc_mcb;
    MCB *next_next_loc_mcb;

    next_loc_mcb = NULL;
    next_next_loc = NULL;
    next_next_loc_mcb = NULL;

    current_loc = firstbyte - sizeof(MCB);
    current_loc_mcb = (MCB *) current_loc;
    current_loc_mcb->available = 1;

    next_loc = firstbyte + current_loc_mcb->block_size;
    if (next_loc != _heap.end) {
        next_loc_mcb = (MCB *) next_loc;
    }

    if (current_loc_mcb->prior_mcb != NULL && current_loc_mcb->prior_mcb->available == 1) {
        current_loc_mcb->prior_mcb->block_size = current_loc_mcb->prior_mcb->block_size + sizeof(MCB) + current_loc_mcb->block_size;
        current_loc_mcb = current_loc_mcb->prior_mcb;
        if (next_loc_mcb != NULL) {
            next_loc_mcb->prior_mcb = current_loc_mcb;
        }
    }

    if (next_loc_mcb != NULL && next_loc_mcb->available == 1) {
        current_loc_mcb->block_size = current_loc_mcb->block_size + sizeof(MCB) + next_loc_mcb->block_size;
        next_next_loc = next_loc + sizeof(MCB) + next_loc_mcb->block_size;
        if (next_next_loc != _heap.end) {
            next_next_loc_mcb = (MCB *) next_next_loc;
            next_next_loc_mcb->prior_mcb = current_loc_mcb;
        }
    }
}

void clear_heap() {
    if (!init) {
        malloc_init();
    }

    void *current_loc;
    MCB *current_loc_mcb;

    current_loc = _heap.start;

    while (current_loc != _heap.end) {
        current_loc_mcb = (MCB *) current_loc;
        /* check if current block is available. */
        if (!current_loc_mcb->available) {
            free(current_loc + sizeof(MCB));
        }
        current_loc = current_loc + sizeof(MCB) + current_loc_mcb->block_size;
    }
}

#endif
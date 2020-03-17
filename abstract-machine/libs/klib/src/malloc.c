//
// Created by Popping Lim on 2020/3/14.
//

#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
extern _Area _heap;

static int has_initialized = 0;
static long long last_index = 0;
void *managed_memory_start;
void *last_valid_address;

struct mem_control_block {
    int is_available;
    int size;
};

void init() {
    managed_memory_start = _heap.start;
    has_initialized = 1;
}

int check_brk(size_t size) {
    if(size + last_index < 0xffffffff)
        return 1;
    return -1;
}

void *malloc(size_t size) {
    assert(size % 8 == 0);
    if(!has_initialized) {
        init();
    }

    void* current_location;
    current_location = managed_memory_start;

    struct mem_control_block* current_location_mcb;
    current_location_mcb = NULL;

    void* memory_location;
    memory_location = NULL;

    size = size + sizeof(struct mem_control_block);

    while(current_location != last_valid_address) {
        if (current_location_mcb->is_available)
            if (current_location_mcb->size >= size) {
                current_location_mcb->is_available = 0;
                memory_location = current_location;
                break;
            }
        current_location = current_location + current_location_mcb->size;
    }

    // No available block in current memory list. Should allocate new memory.
    if (!memory_location) {
        if (check_brk(size) == -1)
            return NULL;
        last_index += size;
        memory_location = last_valid_address;
        last_valid_address += size;
        current_location_mcb = (struct mem_control_block*)memory_location;
        current_location_mcb->is_available = 0;
        current_location_mcb->size = size;
    }
    memory_location = memory_location + sizeof(struct mem_control_block);
    return memory_location;
}

void free(void* ptr) {
    struct mem_control_block* mcb;
    mcb = ptr - sizeof(struct mem_control_block);
    mcb->is_available = 1;
}

#endif
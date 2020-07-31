#include <common.h>

extern _Area _heap;

typedef struct memory_control_block {
    size_t block_size;
    struct memory_control_block *prior_mcb;
    struct memory_control_block *next_mcb;
    int available;
    int align;
    __attribute__((aligned(0x10))) struct {
    } space;
} MCB;


#define align_to(p, a) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))
#define KiB *1024
#define PAGE (4 KiB)
#define alloc512 512
#define alloc1024 256
#define alloc2048 128
#define alloc4096 256


static uintptr_t start;
static uintptr_t end;
static uintptr_t quarter4096;
static uintptr_t quarter2048;
static uintptr_t quarter1024;
static uintptr_t quarter512;


static unsigned int bitmap4096[alloc4096];
static unsigned int bitmap2048[alloc2048];
static unsigned int bitmap1024[alloc1024];
static unsigned int bitmap512[alloc512];

static unsigned int mask = (unsigned int)-1;
static pthread_mutex global_large_lock4096[alloc4096];
static pthread_mutex global_large_lock2048[alloc2048];
static pthread_mutex global_large_lock1024[alloc1024];
static pthread_mutex global_large_lock512[alloc512];

static pthread_mutex global_small_lock;
MCB *head;
MCB *tail;

static inline size_t align(size_t size) {
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;
    return size;
}


static void kfree(void *firstbyte) {
    uintptr_t start_byte = (uintptr_t) firstbyte;
    if (start_byte >= quarter4096) {
        if(start_byte >= quarter512) {
            unsigned int space = (start_byte - quarter512) >> 9;
            unsigned int i = space >> 5;
            unsigned int j = (space << 27) >> 27;
            unsigned int position = 1;
            position <<= j;
            pthread_mutex_lock(&global_large_lock512[i]);
            bitmap512[i] |= position;
            pthread_mutex_unlock(&global_large_lock512[i]);
            return;
        } else if(start_byte >= quarter1024){
            unsigned int space = (start_byte - quarter1024) >> 10;
            unsigned int i = space >> 5;
            unsigned int j = (space << 27) >> 27;
            unsigned int position = 1;
            position <<= j;
            pthread_mutex_lock(&global_large_lock1024[i]);
            bitmap1024[i] |= position;
            pthread_mutex_unlock(&global_large_lock1024[i]);
            return;
        } else if(start_byte >= quarter2048) {
            unsigned int space = (start_byte - quarter2048) >> 11;
            unsigned int i = space >> 5;
            unsigned int j = (space << 27) >> 27;
            unsigned int position = 1;
            position <<= j;
            pthread_mutex_lock(&global_large_lock2048[i]);
            bitmap2048[i] |= position;
            pthread_mutex_unlock(&global_large_lock2048[i]);
            return;
        } else {
            unsigned int space = (start_byte - quarter4096) >> 12;
            unsigned int i = space >> 5;
            unsigned int j = (space << 27) >> 27;
            unsigned int position = 1;
            position <<= j;
            pthread_mutex_lock(&global_large_lock4096[i]);
            bitmap4096[i] |= position;
            pthread_mutex_unlock(&global_large_lock4096[i]);
            return;
        }
    }
    pthread_mutex_lock(&global_small_lock);
    MCB *current_loc_mcb;
    MCB *next_loc_mcb;
    MCB *prior_loc_mcb;
    current_loc_mcb = ((MCB *) (start_byte - sizeof(MCB)));
    if (current_loc_mcb->available == 1) {
        return;
    }
    current_loc_mcb->available = 1;

    next_loc_mcb = current_loc_mcb->next_mcb;
    if (next_loc_mcb != NULL) {
        current_loc_mcb->block_size += next_loc_mcb->align;
        next_loc_mcb->align = 0;
        if (next_loc_mcb->available == 1) {
            current_loc_mcb->block_size = next_loc_mcb->block_size + sizeof(MCB) + current_loc_mcb->block_size;
            next_loc_mcb->next_mcb->prior_mcb = current_loc_mcb;
            current_loc_mcb->next_mcb = next_loc_mcb->next_mcb;
        }
    }

    prior_loc_mcb = current_loc_mcb->prior_mcb;
    if (prior_loc_mcb != NULL) {
        if (prior_loc_mcb->available == 1) {
            prior_loc_mcb->block_size = current_loc_mcb->block_size + sizeof(MCB) + prior_loc_mcb->block_size;
            current_loc_mcb->next_mcb->prior_mcb = prior_loc_mcb;
            prior_loc_mcb->next_mcb = current_loc_mcb->next_mcb;
        }
    }

    pthread_mutex_unlock(&global_small_lock);
}

static inline void *kalloc_small(size_t numbytes) {

    MCB *current_loc_mcb = head;
    MCB *next_loc_mcb;
    MCB *next_next_loc_mcb;
    void *malloc_loc = NULL;
    while (current_loc_mcb != tail) {
        /* check if current block is available. */
        pthread_mutex_lock(&global_small_lock);
        if (current_loc_mcb->available && numbytes + sizeof(MCB) <= current_loc_mcb->block_size) {
            next_next_loc_mcb = current_loc_mcb->next_mcb;
            uintptr_t start_address = (uintptr_t) next_next_loc_mcb - numbytes;
            unsigned int space = start_address % numbytes;
            if(current_loc_mcb->block_size - numbytes - sizeof(MCB) - space < 0) {
                pthread_mutex_unlock(&global_small_lock);
                current_loc_mcb = current_loc_mcb->next_mcb;
                continue;
            }
            current_loc_mcb->block_size = current_loc_mcb->block_size - numbytes - sizeof(MCB) - space;
            next_next_loc_mcb->align = space;
            uintptr_t next_next_loc = (uintptr_t) next_next_loc_mcb - space - numbytes - sizeof(MCB);
            if(next_next_loc < start) {
                pthread_mutex_unlock(&global_small_lock);
                current_loc_mcb = current_loc_mcb->next_mcb;
                continue;
            }
            next_loc_mcb = (MCB *) ((uintptr_t) next_next_loc_mcb - space - numbytes - sizeof(MCB));
            next_loc_mcb->available = 0;
            next_loc_mcb->align = 0;
            next_loc_mcb->block_size = numbytes;
            next_loc_mcb->next_mcb = next_next_loc_mcb;
            next_loc_mcb->prior_mcb = current_loc_mcb;
            current_loc_mcb->next_mcb = next_loc_mcb;
            next_next_loc_mcb->prior_mcb = next_loc_mcb;
            malloc_loc = (void *) ((uintptr_t) next_loc_mcb + sizeof(MCB));
            assert((uintptr_t) malloc_loc % numbytes == 0);
            pthread_mutex_unlock(&global_small_lock);
            return malloc_loc;
        }
        pthread_mutex_unlock(&global_small_lock);
        current_loc_mcb = current_loc_mcb->next_mcb;
    }
    return malloc_loc;
}

static inline void *kalloc_large4096() {
    void *malloc_loc = NULL;
    for (int i = 0; i < alloc4096; i++) {
        if(pthread_mutex_trylock(&global_large_lock4096[i])) continue;
        unsigned int position = bitmap4096[i] & (-bitmap4096[i]);
        if (position == 0) {
            pthread_mutex_unlock(&global_large_lock4096[i]);
            continue;
        }
        bitmap4096[i] &= (~position);
        int j = __builtin_ctz(position);
        malloc_loc = (void *) (quarter4096 + (((i << 5) + j) << 12));
        pthread_mutex_unlock(&global_large_lock4096[i]);
        return malloc_loc;
    }
    return malloc_loc;
}

static inline void *kalloc_large2048() {
    void *malloc_loc = NULL;
    for (int i = 0; i < alloc2048; i++) {
        if(pthread_mutex_trylock(&global_large_lock2048[i])) continue;
        unsigned int position = bitmap2048[i] & (-bitmap2048[i]);
        if (position == 0) {
            pthread_mutex_unlock(&global_large_lock2048[i]);
            continue;
        }
        bitmap2048[i] &= (~position);
        int j = __builtin_ctz(position);
        malloc_loc = (void *) (quarter2048 + (((i << 5) + j) << 11));
        pthread_mutex_unlock(&global_large_lock2048[i]);
        return malloc_loc;
    }

    for (int i = 0; i < alloc2048; i++) {
        if(pthread_mutex_trylock(&global_large_lock2048[i])) continue;
        unsigned int position = bitmap2048[i] & (-bitmap2048[i]);
        if (position == 0) {
            pthread_mutex_unlock(&global_large_lock2048[i]);
            continue;
        }
        bitmap2048[i] &= (~position);
        int j = __builtin_ctz(position);
        malloc_loc = (void *) (quarter2048 + (((i << 5) + j) << 11));
        pthread_mutex_unlock(&global_large_lock2048[i]);
        return malloc_loc;
    }
    return kalloc_large4096();
}

static inline void *kalloc_large1024() {
    void *malloc_loc = NULL;
    for (int i = 0; i < alloc1024; i++) {
        if(pthread_mutex_trylock(&global_large_lock1024[i])) continue;
        unsigned int position = bitmap1024[i] & (-bitmap1024[i]);
        if (position == 0) {
            pthread_mutex_unlock(&global_large_lock1024[i]);
            continue;
        }
        bitmap1024[i] &= (~position);
        int j = __builtin_ctz(position);
        malloc_loc = (void *) (quarter1024 + (((i << 5) + j) << 10));
        pthread_mutex_unlock(&global_large_lock1024[i]);
        return malloc_loc;
    }

    for (int i = 0; i < alloc1024; i++) {
        if(pthread_mutex_trylock(&global_large_lock1024[i])) continue;
        unsigned int position = bitmap1024[i] & (-bitmap1024[i]);
        if (position == 0) {
            pthread_mutex_unlock(&global_large_lock1024[i]);
            continue;
        }
        bitmap1024[i] &= (~position);
        int j = __builtin_ctz(position);
        malloc_loc = (void *) (quarter1024 + (((i << 5) + j) << 10));
        pthread_mutex_unlock(&global_large_lock1024[i]);
        return malloc_loc;
    }
    return kalloc_large2048();
}

static inline void *kalloc_large512() {
    void *malloc_loc = NULL;
    for (int i = 0; i < alloc512; i++) {
        if(pthread_mutex_trylock(&global_large_lock512[i])) continue;
        unsigned int position = bitmap512[i] & (-bitmap512[i]);
        if (position == 0) {
            pthread_mutex_unlock(&global_large_lock512[i]);
            continue;
        }
        bitmap512[i] &= (~position);
        int j = __builtin_ctz(position);
        malloc_loc = (void *) (quarter512 + (((i << 5) + j) << 9));
        pthread_mutex_unlock(&global_large_lock512[i]);
        return malloc_loc;
    }

    for (int i = 0; i < alloc512; i++) {
        if(pthread_mutex_trylock(&global_large_lock512[i])) continue;
        unsigned int position = bitmap512[i] & (-bitmap512[i]);
        if (position == 0) {
            pthread_mutex_unlock(&global_large_lock512[i]);
            continue;
        }
        bitmap512[i] &= (~position);
        int j = __builtin_ctz(position);
        malloc_loc = (void *) (quarter512 + (((i << 5) + j) << 9));
        pthread_mutex_unlock(&global_large_lock512[i]);
        return malloc_loc;
    }
    return kalloc_large1024();
}


static void *kalloc(size_t numbytes) {
    if (numbytes > 2048) {
        return kalloc_large4096();
    } else if (numbytes > 1024) {
        return kalloc_large2048();
    } else if (numbytes > 512){
        return kalloc_large1024();
    } else if (numbytes > 256) {
        return kalloc_large512();
    } else if(numbytes < 16) {
        numbytes = 16;
    } else{
        numbytes = align(numbytes);
    }

    return kalloc_small(numbytes);
}

static void pmm_init() {
    start = align_to((uintptr_t) _heap.start + 16, PAGE);
    end = (uintptr_t) _heap.end;
    quarter4096 = start + (end - start) / 9 * 4;
    quarter4096 = align_to(quarter4096, PAGE);

    head = (MCB *) (start - sizeof(MCB));
    tail = (MCB *) (quarter4096 - sizeof(MCB));
    head->available = 1;
    head->align = 0;
    head->block_size = quarter4096 - sizeof(MCB) - start;
    head->next_mcb = tail;
    head->prior_mcb = tail;
    tail->available = 0;
    tail->align = 0;
    tail->block_size = 0;
    tail->next_mcb = head;
    tail->prior_mcb = head;

    quarter2048 = quarter4096 + 4096 * alloc4096 * 32;
    quarter1024 = quarter2048 + 2048 * alloc2048 * 32;
    quarter512 = quarter1024 + 1024 * alloc1024 * 32;

    for(int i = 0; i < alloc512; i++) {
        bitmap512[i] = mask;
        global_large_lock512[i] = 0;
    }
    for(int i = 0; i < alloc1024; i++) {
        bitmap1024[i] = mask;
        global_large_lock1024[i] = 0;
    }
    for(int i = 0; i < alloc2048; i++) {
        bitmap2048[i] = mask;
        global_large_lock2048[i] = 0;
    }
    for(int i = 0; i < alloc4096; i++) {
        bitmap4096[i] = mask;
        global_large_lock4096[i] = 0;
    }
    global_small_lock = 0;
    panic_on((quarter512 + 512 * alloc512 * 32) > end, "pool too large");
}

static void *kalloc_safe(size_t size) {
    int i = _intr_read();
    _intr_write(0);
    void *ret = kalloc(size);
    panic_on(ret < _heap.start || ret > _heap.end, "alloc error!");
    if (i) _intr_write(1);
    return ret;
}

static void kfree_safe(void *ptr) {
    int i = _intr_read();
    _intr_write(0);
    kfree(ptr);
    if (i) _intr_write(1);
}

MODULE_DEF(pmm) = {
        .init  = pmm_init,
        .alloc = kalloc_safe,
        .free  = kfree_safe,
};

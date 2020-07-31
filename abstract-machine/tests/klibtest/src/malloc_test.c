//
// Created by kruskallin on 2020/3/22.
//

#include <klib.h>
#include <klib-macros.h>

extern _Area _heap;


static void kernel(void (*k)(void), const char *name) {
    printf("Testing %s...\n", name);
    k();
}

void test_malloc1() {
    clear_heap();
    void* test_size1 = malloc(16);
    free(test_size1);
    malloc(16);
    MCB* mcb = (MCB*)(_heap.start + sizeof(MCB) + 16);
    assert(mcb->available == 1);
}

void test_malloc2() {
    clear_heap();
    void* test_size1 = malloc(16);
    void* test_size2 = malloc(16);
    void* test_size3 = malloc(16);
    MCB* mcb1 = (MCB*)(test_size1 - sizeof(MCB));
    MCB* mcb2 = (MCB*)(test_size2 - sizeof(MCB));
    MCB* mcb3 = (MCB*)(test_size3 - sizeof(MCB));
    assert(mcb2->prior_mcb == mcb1);
    assert(mcb3->prior_mcb == mcb2);
    free(test_size2);
    assert(mcb2->available == 1);
}

void test_malloc3() {
    clear_heap();
    void* test_size1 = malloc(16);
    MCB* mcb2 = (MCB*)(test_size1 + 16);
    assert(mcb2->block_size == (_heap.end - _heap.start - 2 * sizeof(MCB) - 16));
}

void test_malloc4() {
    clear_heap();
    void* test_size1 = malloc(16);
    void* test_size2 = malloc(16);
    void* test_size3 = malloc(16);
    MCB* mcb1 = (MCB*)(test_size1 - sizeof(MCB));
    MCB* mcb2 = (MCB*)(test_size2 - sizeof(MCB));
    MCB* mcb3 = (MCB*)(test_size3 - sizeof(MCB));
    MCB* mcb4 = (MCB*)(test_size3 + 16);
    assert(mcb1->block_size == 16);
    assert(mcb2->block_size == 16);
    assert(mcb3->block_size == 16);
    assert(mcb4->block_size == (_heap.end - _heap.start - 4 * sizeof(MCB) - 3 * 16));
    assert(mcb2->prior_mcb == mcb1);
    free(test_size1);
    free(test_size2);
    assert(mcb1->available == 1);
    assert(mcb1->block_size == (sizeof(MCB) + 16 + 16));
    assert(mcb3->prior_mcb == mcb1);
    free(test_size3);
    assert(mcb1->block_size == (_heap.end - _heap.start - sizeof(MCB)));
}

void malloc_test() {
    kernel(test_malloc1, "malloc1");
    kernel(test_malloc2, "malloc2");
    kernel(test_malloc3, "malloc3");
    kernel(test_malloc4, "malloc4");

}

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "co-test.h"
extern co_num;
int g_count = 0;
static struct co *co_array[128];


static void add_count() {
    g_count++;
}

static int get_count() {
    return g_count;
}

static void work_loop(void *arg) {
    const char *s = (const char*)arg;
    for (int i = 0; i < 1024; ++i) {
//        printf("%s%d  ", s, get_count());
        add_count();
        co_yield();
    }
}

static void work(void *arg) {
    work_loop(arg);
}

static void test_1() {

//    struct co *thd1 = co_start("thread-1", work, "X");
//    struct co *thd2 = co_start("thread-2", work, "Y");

//    co_wait(thd1);
//    co_wait(thd2);

    for(int i = 0; i < 100; i++) {
        for(int j = 0; j < 128; j++) {
            co_array[j] = co_start("thread", work, "A");
        }
        for(int j = 0; j < 128; j++) {
            co_wait(co_array[j]);
        }
    }

//    printf("\n");
}

// -----------------------------------------------

static int g_running = 1;

static void do_produce(Queue *queue) {
    assert(!q_is_full(queue));
    Item *item = (Item*)malloc(sizeof(Item));
    if (!item) {
        fprintf(stderr, "New item failure\n");
        return;
    }
    item->data = (char*)malloc(10);
    if (!item->data) {
        fprintf(stderr, "New data failure\n");
        free(item);
        return;
    }
    memset(item->data, 0, 10);
    sprintf(item->data, "libco-%d", g_count++);
    q_push(queue, item);
}

static void producer(void *arg) {
    Queue *queue = (Queue*)arg;
    for (int i = 0; i < 100; ) {
        if (!q_is_full(queue)) {
//             co_yield();
            do_produce(queue);
            i += 1;
        }
        co_yield();
    }
}

static void do_consume(Queue *queue) {
    assert(!q_is_empty(queue));

    Item *item = q_pop(queue);
    if (item) {
//        printf("%s  ", (char *)item->data);
        free(item->data);
        free(item);
    }
}

static void consumer(void *arg) {
    Queue *queue = (Queue*)arg;
    while (g_running) {
        if (!q_is_empty(queue)) {
            do_consume(queue);
        }
        co_yield();
    }
}

static void test_2() {

    Queue *queue = q_new();

//    struct co *thd1 = co_start("producer-1", producer, queue);
//    struct co *thd2 = co_start("producer-2", producer, queue);
//    struct co *thd3 = co_start("consumer-1", consumer, queue);
//    struct co *thd4 = co_start("consumer-2", consumer, queue);

    for(int j = 0; j < 16; j++) {
        g_running = 1;
        for (int i = 0; i < 64; i++) {
            co_array[i] = co_start("producer", producer, queue);
        }

        for (int i = 64; i < 128; i++) {
            co_array[i] = co_start("consumer", consumer, queue);
        }
        printf("%d\n", co_num);
//    co_wait(thd1);
//    co_wait(thd2);
        for (int i = 0; i < 64; i++) {
            co_wait(co_array[i]);
        }

        g_running = 0;

        for (int i = 64; i < 128; i++) {
            co_wait(co_array[i]);
        }
    }
//    co_wait(thd3);
//    co_wait(thd4);

    while (!q_is_empty(queue)) {
        do_consume(queue);
    }

    q_free(queue);
}


static void random_output(int *k) {
//    int temp = rand();
//    printf("%d\n", temp);
    for (int i = 0; i < rand() % 1024; i++) {
        printf("random: %d\n", *k);
        co_yield();
    }
}

static void test_3() {
    int arr[128];
    for(int i = 0; i < 128; i++) {
        arr[i] = i;
    }
    for(int i = 0; i < 1; i++) {
        for(int j = 0; j < 128; j++) {
            co_array[j] = co_start("random", random_output, &arr[j]);
        }
        for(int j = 0; j < 128; j++) {
            co_wait(co_array[j]);
        }
    }
}

int count = 1; // 协程之间共享

static void entry(void *arg) {
    for (int i = 0; i < 1000; i++) {
        printf("%s[%d] ", (const char *)arg, count++);
        co_yield();
    }
}

static int test_4() {
    struct co *co1 = co_start("co1", entry, "a");
    struct co *co2 = co_start("co2", entry, "b");
    co_wait(co1);
    co_wait(co2);
    printf("Done\n");
}

int main() {
    setbuf(stdout, NULL);

//    printf("Test #1. Expect: (X|Y){0, 1, 2, ..., 199}\n");
    test_1();

//    printf("\n\nTest #2. Expect: (libco-){200, 201, 202, ..., 399}\n");
//    test_2();

//    printf("\n\nTest #3.\n");
//    test_3();

//    test_4();

//    printf("\n\n");

    return 0;
}

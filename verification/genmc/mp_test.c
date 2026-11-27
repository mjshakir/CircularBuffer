// GenMC model of the library's test MultipleProducersSingleConsumer (test/CircularBufferDynamicTest.cpp:487 and
// CircularBufferFixedTest.cpp:447): two producers push, the first one to finish sets `ready`, the consumer waits
// for `ready`, then loops   while (consumed < MAX && !empty()) { if (pop()) ++consumed; }   and the test
// finally expects empty().
//   CAP       capacity                                               [1]
//   PROD_OPS  pushes per producer                                    [2]
//   WAIT_BOTH 0: the consumer starts when the FIRST producer is done (the test); 1: when BOTH are done [0]
//   WHAT      0: the test's end condition, empty(); 1: the accounting  count == pushes - pops - evictions
#define GHOST 1
#ifdef RING_SEQ
#include "ring_seq.h"
#else
#include "ring.h"
#endif

#ifndef PROD_OPS
#define PROD_OPS 2
#endif
#ifndef WAIT_BOTH
#define WAIT_BOTH 0
#endif
#ifndef WHAT
#define WHAT 0
#endif

static atomic_int  finished; // producers that have returned
static atomic_int  ready;    // set by each producer when it is done (the test's flag, under its mutex)
static int         pops_done;

static void *producer(void *arg) {
    const int base = (int)(long)arg * 10;
    for (int k = 0; k < PROD_OPS; ++k) {
        push(base + k + 1);
    }
    atomic_fetch_add_explicit(&finished, 1, memory_order_seq_cst);
    atomic_fetch_add_explicit(&ready, 1, memory_order_seq_cst); // the test sets ready under its mutex; a counter keeps the writes ordered for GenMC
    return arg;
}

static void *consumer(void *arg) {
#if WAIT_BOTH
    while (atomic_load_explicit(&finished, memory_order_seq_cst) < 2) {
    }
#else
    while (atomic_load_explicit(&ready, memory_order_seq_cst) < 1) {
    }
#endif
    int consumed = 0;
    while (consumed < 2 * PROD_OPS && !is_empty()) {
        if (pop()) {
            ++consumed;
        }
    }
    pops_done = consumed;
    return arg;
}

int main(void) {
    set_up();
    pthread_t p1, p2, c;
    pthread_create(&p1, 0, producer, (void *)0);
    pthread_create(&p2, 0, producer, (void *)1);
    pthread_create(&c, 0, consumer, 0);
    pthread_join(p1, 0);
    pthread_join(p2, 0);
    pthread_join(c, 0);
#if WHAT == 0
    assert(is_empty()); // EXPECT_TRUE(buffer.empty())
#else
    const unsigned long count     = count_now();
    const unsigned long evictions = atomic_load_explicit(&g_evictions, memory_order_relaxed);
    assert(count <= CAP);
    assert((long)count == (long)(2 * PROD_OPS) - pops_done - (long)evictions);
#endif
    return 0;
}

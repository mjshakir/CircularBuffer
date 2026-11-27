// GenMC scenario for the ring of ring.h: producers, consumers and an observer run the header's functions
// concurrently on a small buffer, then main checks the whole run.
//
// Scenario (all -D, defaults in brackets):
//   CAP        capacity 1..3                                                  [2]
//   PREFILL    items pushed by main before the threads start, 0..3            [0]
//   NPROD      producer threads 0..2, each with PROD_OPS operations           [1] [3]
//   PROD_KIND  0 try_emplace, 1 push(const T&), 2 push(T&&), 3 emplace        [0]
//   NCONS      consumer threads 0..2, each with CONS_OPS operations           [1] [3]
//   CONS_KIND  0 top_pop, 1 consume, 2 pop, 3 top() then pop()                [1]
//   OBSERVER   a thread reading size(), empty(), full()                       [0]
//   CONS_LAST  the consumer also calls last() after each operation            [0]
//   PROD_TOP   the producer also calls top() after each operation             [0]
//   PROD_LAST  the producer also calls last() after each operation            [0]
//   AFTER_PUSH main pushes one more item after the join, so a head or tail left
//              in a wrong place shows as a lost, repeated or unwritten item             [0]
//   CLEAR_AT_END  main calls clear() after the join and checks it             [0]
//
// Values: prefill item i is 1 + i, the k-th item of producer t is 10 * (t + 2) + k, so value / 10 names the
// producer (0: prefill) and the order inside one producer is the order of the value.
// Checked: every slot access is race free (GenMC), every value taken is one that was put in, none is taken twice,
// the order of one producer is kept, head and tail stay below the capacity, the count never passes the capacity,
// at the end count == accepted - taken - evicted, try_emplace never evicts, a lost item is older than every
// item that stayed, and a buffer left alone comes out empty.
#define GHOST 1
#ifdef RING_SEQ
#include "ring_seq.h"
#else
#include "ring.h"
#endif

#ifndef PREFILL
#define PREFILL 0
#endif
#ifndef NPROD
#define NPROD 1
#endif
#ifndef PROD_OPS
#define PROD_OPS 3
#endif
#ifndef PROD_KIND
#define PROD_KIND 0
#endif
#ifndef NCONS
#define NCONS 1
#endif
#ifndef CONS_OPS
#define CONS_OPS 3
#endif
#ifndef CONS_KIND
#define CONS_KIND 1
#endif
#ifndef OBSERVER
#define OBSERVER 0
#endif
#ifndef CONS_LAST
#define CONS_LAST 0
#endif
#ifndef PROD_TOP
#define PROD_TOP 0
#endif
#ifndef AFTER_PUSH
#define AFTER_PUSH 0
#endif
#ifndef PROD_LAST
#define PROD_LAST 0
#endif
#ifndef CLEAR_AT_END
#define CLEAR_AT_END 0
#endif
#ifndef STRICT_SIZE
#define STRICT_SIZE 1
#endif

#define VALUE(t, k) (10 * ((t) + 2) + (k))

static int prod_accepted[2][PROD_OPS + 1]; // thread-owned until joined
static int cons_value[2][CONS_OPS + 1];    // 0: nothing taken (or a pop, which reads nothing)
static int cons_took[2][CONS_OPS + 1];     // 1: the operation took an item
static int cons_taken[2];                  // items taken by consumer c

static void producer_op(int t, int k) {
    const int value = VALUE(t, k);
    int       done  = 1;
#if PROD_KIND == 0
    done = try_emplace(value);
    prod_accepted[t][k] = done;
    if (!done && NPROD == 1) {
        // refused only when a full capacity went in: the items put in so far, with the prefill, reach it
        int put = PREFILL;
        for (int j = 0; j < k; ++j) {
            put += prod_accepted[t][j];
        }
        assert(put >= CAP);
    }
#elif PROD_KIND == 1
    push(value);
    prod_accepted[t][k] = 1;
#elif PROD_KIND == 2
    push_mv(value);
    prod_accepted[t][k] = 1;
#else
    emplace(value);
    prod_accepted[t][k] = 1;
#endif
    (void)done;
#if STRICT_SIZE
    assert(get_size() <= CAP);
#endif
#if PROD_TOP
    int seen_top = 0;
    if (top(&seen_top)) {
        assert(seen_top > 0);
    }
#endif
#if PROD_LAST
    int seen_last = 0;
    if (last(&seen_last)) {
        assert(seen_last > 0);
    }
#endif
}

static void *producer(void *arg) {
    const int t = (int)(long)arg;
    for (int k = 0; k < PROD_OPS; ++k) {
        producer_op(t, k);
    }
    return arg;
}

static void consumer_op(int c, int i) {
    int value = 0;
    int took  = 0;
#if CONS_KIND == 0
    took = top_pop(&value);
#elif CONS_KIND == 1
    took = consume(&value);
#elif CONS_KIND == 2
    took = pop();
#else
    if (top(&value)) {
        took = pop();
        if (!took) {
            value = 0; // another thread took it between the two calls: nothing is ours
        }
    }
#endif
    if (took) {
#if CONS_KIND != 2
        assert(value > 0); // a value that was written and not destroyed
#endif
        cons_value[c][i] = value;
        cons_took[c][i]  = 1;
        cons_taken[c] += 1;
    }
#if STRICT_SIZE
    assert(get_size() <= CAP);
#endif
#if CONS_LAST
    int seen = 0;
    if (last(&seen)) {
        assert(seen > 0);
    }
#endif
}

static void *consumer(void *arg) {
    const int c = (int)(long)arg;
    for (int i = 0; i < CONS_OPS; ++i) {
        consumer_op(c, i);
    }
    return arg;
}

static void *observer(void *arg) {
    for (int look = 0; look < 2; ++look) {
        assert(get_size() <= CAP);
        (void)is_empty();
        (void)full();
    }
    return arg;
}

// one value, checked as "valid and not yet seen"
static int  seen_flag[64];
static int  valid_flag[64];
static void take(int value) {
    assert(value > 0 && value < 64);
    assert(valid_flag[value]);  // it was put in
    assert(!seen_flag[value]);  // and comes out once
    seen_flag[value] = 1;
}

// the values of one band (producer) in one sequence must ascend
static void ascending(const int *seq, int n) {
    int last_of_band[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < n; ++i) {
        if (seq[i] == 0) {
            continue;
        }
        const int band = seq[i] / 10;
        assert(seq[i] > last_of_band[band]);
        last_of_band[band] = seq[i];
    }
}

int main(void) {
    set_up();
    int accepted_total = 0;
    for (int i = 0; i < PREFILL; ++i) {
        push(1 + i);
        valid_flag[1 + i] = 1;
        accepted_total++;
    }
    pthread_t threads[NPROD + NCONS + OBSERVER + 1];
    int       n = 0;
    for (long t = 0; t < NPROD; ++t) {
        pthread_create(&threads[n++], 0, producer, (void *)t);
    }
    for (long c = 0; c < NCONS; ++c) {
        pthread_create(&threads[n++], 0, consumer, (void *)c);
    }
#if OBSERVER
    pthread_create(&threads[n++], 0, observer, 0);
#endif
    for (int j = 0; j < n; ++j) {
        pthread_join(threads[j], 0);
    }

    // every thread is joined: the whole state is ours
    for (int t = 0; t < NPROD; ++t) {
        for (int k = 0; k < PROD_OPS; ++k) {
            if (prod_accepted[t][k]) {
                valid_flag[VALUE(t, k)] = 1;
                accepted_total++;
            }
        }
    }
#if AFTER_PUSH
    push(40);
    valid_flag[40] = 1;
    accepted_total++;
#endif
    const unsigned long head_end  = head_pos();
    const unsigned long tail_end  = tail_pos();
    const unsigned long count_end = count_now();
    const unsigned long evictions = atomic_load_explicit(&g_evictions, memory_order_relaxed);
    assert(head_end < CAP && tail_end < CAP); // head and tail never leave their range
    assert(count_end <= CAP);                 // the count never passes the capacity

    int taken_total = 0;
    for (int c = 0; c < NCONS; ++c) {
        taken_total += cons_taken[c];
        for (int i = 0; i < CONS_OPS; ++i) {
            if (cons_value[c][i] != 0) {
                take(cons_value[c][i]);
            }
        }
        ascending(cons_value[c], CONS_OPS);
    }
    assert((long)count_end == (long)accepted_total - taken_total - (long)evictions); // the count equation
#if PROD_KIND == 0
    assert(evictions == 0); // try_emplace never removes an item
#endif

    // what is left comes out, oldest first, then the buffer is empty
    int remaining[CAP + 3];
    int remaining_n = 0;
    int v           = 0;
    while (remaining_n < CAP + 2 && top_pop(&v)) {
        remaining[remaining_n++] = v;
    }
    assert(remaining_n == (int)count_end);
    assert(!top_pop(&v));
    assert(is_empty() && get_size() == 0);
    for (int i = 0; i < remaining_n; ++i) {
        take(remaining[i]);
    }
    ascending(remaining, remaining_n);
#if CONS_KIND != 2
    if (NCONS == 1) { // one consumer sees the order of one producer, across the end of the run too
        int joined[CONS_OPS + CAP + 3];
        int m = 0;
        for (int i = 0; i < CONS_OPS; ++i) {
            joined[m++] = cons_value[0][i];
        }
        for (int i = 0; i < remaining_n; ++i) {
            joined[m++] = remaining[i];
        }
        ascending(joined, m);
    }
    // a lost item (put in, never taken, never left) is older than everything left, for one producer
    if (NPROD + (PREFILL > 0) <= 1 && PROD_KIND != 0) {
        for (int value = 1; value < 64; ++value) {
            if (valid_flag[value] && !seen_flag[value]) {
                for (int i = 0; i < remaining_n; ++i) {
                    assert(remaining[i] / 10 != value / 10 || value < remaining[i]);
                }
            }
        }
    }
#endif
#if CLEAR_AT_END
    push(77);
    clear();
    assert(is_empty());
#endif
    return 0;
}

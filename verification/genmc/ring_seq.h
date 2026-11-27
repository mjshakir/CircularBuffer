// GenMC model of a PROPOSED redesign of the ring's concurrent core: one sequence number per slot (the bounded
// many-producer many-consumer queue of Vyukov), with overwrite-on-full done by taking the oldest item out as an
// ordinary consumer would. This is NOT a transcription of include/CircularBuffer.hpp; it is the smallest design
// the model checker accepts on the scenarios where the header fails. It offers the API names of ring.h so that
// scenario.c, sequential.c and mp_test.c run on it unchanged (-DRING_SEQ).
//
// Idea: a slot says by its own number whose turn it is.
//   seq[i] == 2 * ticket              the slot is free for the producer that holds this ticket
//   seq[i] == 2 * ticket + 1          the slot holds the item of that ticket, ready for the consumer with that ticket
//   seq[i] == 2 * (ticket + CAP)      the consumer is done, the slot is free for the next lap
// (the factor 2 keeps "free" and "filled" apart for a capacity of 1; the usual Vyukov queue needs capacity >= 2)
// A producer claims a ticket only when the slot's number says "free for me" (compare-and-swap on the tail),
// writes the slot, then publishes it with ONE release store of the number. A consumer claims a ticket only when
// the number says "holds my item", reads the slot, then frees it with ONE release store. The count is not stored:
// size = tail - head. So the claim and the permission are the same word, and the three failures of the header
// (a consumer claims what is not there, a consumer reads a slot not yet written, a producer writes a slot
// still in use) cannot happen.
//
//   push() on a full buffer takes the oldest item out (one dequeue) and retries: it removes an item only when one
//   is really there at that moment, never "one more than the count said".
//   try_emplace() refuses when the slot of its ticket is not free (never removes anything).
//   Cost per operation: one acquire load of the slot's number, one compare-and-swap on the head or tail, one
//   release store of the number; per slot: one size_t. The count (fetch_add / fetch_sub, two contended RMWs per
//   item in the header) is gone.
#ifndef RING_SEQ_H
#define RING_SEQ_H

#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>

#ifndef CAP
#define CAP 2
#endif
#ifndef DTOR_WRITES
#define DTOR_WRITES 1
#endif
#ifndef GHOST
#define GHOST 0
#endif
#ifndef SLOTS_ATOMIC
#define SLOTS_ATOMIC 0
#endif
#define DESTROYED (-1)
#define UNWRITTEN 0
#if CAP > 0
#define SLOTS CAP
#else
#define SLOTS 1
#endif

#ifndef O_SEQ_LOAD
#define O_SEQ_LOAD memory_order_acquire
#endif
#ifndef O_SEQ_STORE
#define O_SEQ_STORE memory_order_release
#endif
#ifndef O_TICKET_CAS
#define O_TICKET_CAS memory_order_relaxed
#endif

#if SLOTS_ATOMIC
static atomic_int slot[SLOTS];
#define SLOT_RD(i)    atomic_load_explicit(&slot[(i)], memory_order_seq_cst)
#define SLOT_WR(i, v) atomic_store_explicit(&slot[(i)], (v), memory_order_seq_cst)
#else
static int slot[SLOTS];
#define SLOT_RD(i)    slot[(i)]
#define SLOT_WR(i, v) (slot[(i)] = (v))
#endif

static atomic_ulong q_head;     // the ticket of the next item to take
static atomic_ulong q_tail;     // the ticket of the next slot to fill
static atomic_ulong seq[SLOTS]; // per slot: whose turn it is
static atomic_ulong g_evictions;

static unsigned long head_pos(void)  { return atomic_load_explicit(&q_head, memory_order_relaxed) % (CAP == 0 ? 1 : CAP); }
static unsigned long tail_pos(void)  { return atomic_load_explicit(&q_tail, memory_order_relaxed) % (CAP == 0 ? 1 : CAP); }
static unsigned long count_now(void) { return atomic_load_explicit(&q_tail, memory_order_relaxed) - atomic_load_explicit(&q_head, memory_order_relaxed); }

static int set_up(void) {
    for (unsigned long i = 0; i < SLOTS; ++i) {
        SLOT_WR(i, UNWRITTEN);
        atomic_init(&seq[i], 2 * i);
    }
    atomic_init(&q_head, 0UL);
    atomic_init(&q_tail, 0UL);
    return 0;
}

static void destroy(unsigned long index) {
#if DTOR_WRITES
    SLOT_WR(index, DESTROYED);
#else
    (void)index;
#endif
}

// one producer step: claim the slot whose number says "free for my ticket", write it, publish it
static int try_enqueue(int value) {
    if (CAP == 0) {
        return 0;
    }
    unsigned long pos = atomic_load_explicit(&q_tail, memory_order_relaxed);
    for (;;) {
        const unsigned long idx  = pos % CAP;
        const unsigned long turn = atomic_load_explicit(&seq[idx], O_SEQ_LOAD);
        const long          diff = (long)turn - (long)(2 * pos);
        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&q_tail, &pos, pos + 1, O_TICKET_CAS, memory_order_relaxed)) {
                SLOT_WR(idx, value);
                atomic_store_explicit(&seq[idx], 2 * pos + 1, O_SEQ_STORE);
                return 1;
            }
        } else if (diff < 0) {
            return 0; // the slot of this ticket still holds the item of the lap before: full
        } else {
            pos = atomic_load_explicit(&q_tail, memory_order_relaxed);
        }
    }
}

// one consumer step: claim the slot whose number says "holds the item of my ticket", read it, free it
static int dequeue(int *out) {
    if (CAP == 0) {
        return 0;
    }
    unsigned long pos = atomic_load_explicit(&q_head, memory_order_relaxed);
    for (;;) {
        const unsigned long idx  = pos % CAP;
        const unsigned long turn = atomic_load_explicit(&seq[idx], O_SEQ_LOAD);
        const long          diff = (long)turn - (long)(2 * pos + 1);
        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&q_head, &pos, pos + 1, O_TICKET_CAS, memory_order_relaxed)) {
                if (out != NULL) {
                    *out = SLOT_RD(idx);
                }
                destroy(idx);
                atomic_store_explicit(&seq[idx], 2 * (pos + CAP), O_SEQ_STORE);
                return 1;
            }
        } else if (diff < 0) {
            return 0; // the slot does not hold the item of this ticket yet: empty
        } else {
            pos = atomic_load_explicit(&q_head, memory_order_relaxed);
        }
    }
}

static void push(int item) {
    if (CAP == 0) {
        return;
    }
    while (!try_enqueue(item)) {
        if (dequeue(NULL)) { // overwrite on full: the oldest item that is really there goes
#if GHOST
            atomic_fetch_add_explicit(&g_evictions, 1UL, memory_order_relaxed);
#endif
        }
    }
}
static void push_mv(int item) { push(item); }
static void emplace(int arg)  { push(arg); }
static int  try_emplace(int arg) { return try_enqueue(arg); }
static int  pop(void)            { return dequeue(NULL); }
static int  top_pop(int *out)    { return dequeue(out); }
static int  consume(int *out)    { return dequeue(out); } // read where it sits, then free: the same step

static unsigned long get_size(void) {
    const unsigned long head = atomic_load_explicit(&q_head, memory_order_acquire);
    const unsigned long tail = atomic_load_explicit(&q_tail, memory_order_acquire);
    const unsigned long size = tail - head;
    return size > CAP ? CAP : size; // an estimate while threads run, exact when none does
}
static int is_empty(void) { return get_size() == 0; }
static int full(void)     { return get_size() >= CAP; }
static unsigned long get_capacity(void) { return CAP; }

// reading without removing: for the consumer thread only, as the header's top() is in practice
static int top(int *out) {
    const unsigned long pos  = atomic_load_explicit(&q_head, memory_order_acquire);
    const unsigned long turn = atomic_load_explicit(&seq[pos % (CAP == 0 ? 1 : CAP)], O_SEQ_LOAD);
    if (CAP == 0 || turn != 2 * pos + 1) {
        return 0; // nothing published at the head
    }
    *out = SLOT_RD(pos % CAP);
    return 1;
}
static int last(int *out) {
    const unsigned long tail = atomic_load_explicit(&q_tail, memory_order_acquire);
    if (CAP == 0 || tail == 0) {
        return 0;
    }
    const unsigned long pos  = tail - 1;
    const unsigned long turn = atomic_load_explicit(&seq[pos % CAP], O_SEQ_LOAD);
    if (turn != 2 * pos + 1) {
        return 0; // the newest ticket is not published yet
    }
    *out = SLOT_RD(pos % CAP);
    return 1;
}
static void clear(void) {
    while (!is_empty()) {
        if (!pop()) {
            break;
        }
    }
}

#endif // RING_SEQ_H

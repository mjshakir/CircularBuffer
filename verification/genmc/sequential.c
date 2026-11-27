// GenMC model, one thread: the buffer of ring.h against a reference queue, and the lifetime observations.
//
// SCRIPT is a string of operations, one letter each: t try_emplace, p push(const T&), m push(T&&), e emplace,
// o pop, q top_pop, c consume, x clear. (GenMC keeps every event of an execution in one graph, so a script is
// short and each arm runs one script; the scripts together cover fill, refuse, overwrite, wrap and drain.)
// After each operation: the answer equals the reference queue's, top() is the oldest item, last() the newest,
// size() the length, head and tail are below the capacity, the count is at most the capacity, an evicting push
// drops exactly the oldest item, try_emplace on a full buffer refuses and changes nothing, the count equals
// pushes - pops - evictions.
//
//   CAP 0     a buffer without slots: every call answers, none stores, no slot is touched
//   SCRIPT    the operations of one run
//   OBS       0: the sequences; 1: pop then push(T&&) into the slot the destructor ran on (operator= on a destroyed
//             object); 2: push, pop, then the container's own destructor (a second destructor on one object);
//             3: as 1 but the second item is emplaced (placement new on a destroyed slot is fine);
//             4: a full buffer, push evicts the oldest, the new item is assigned into the evicted slot
#define GHOST 1
#ifdef RING_SEQ
#include "ring_seq.h"
#else
#include "ring.h"
#endif

#ifndef SCRIPT
#define SCRIPT tttqq
#endif
#define STR_(x) #x
#define STR(x) STR_(x)
#ifndef OBS
#define OBS 0
#endif

static int ref[8];
static int ref_n;
static int next_value;
static unsigned long pushes, pops, evicted;

static void check_state(void) {
#if CAP > 0
    assert(head_pos() < CAP);
    assert(tail_pos() < CAP);
#endif
    assert(count_now() <= CAP);
#if CAP == 0
    assert(SLOT_RD(0) == UNWRITTEN); // a buffer without slots touches none
#endif
    assert(get_size() == (unsigned long)ref_n);
    assert(is_empty() == (ref_n == 0));
    assert(full() == (ref_n == CAP));
    assert(get_capacity() == CAP);
    assert(pushes - pops - evicted == (unsigned long)ref_n);
    assert(atomic_load_explicit(&g_evictions, memory_order_relaxed) == evicted);
    int v = 0;
    if (ref_n == 0) {
        assert(!top(&v) && !last(&v)); // an empty buffer never yields a value
    } else {
        assert(top(&v) && v == ref[0]);
        assert(last(&v) && v == ref[ref_n - 1]);
    }
}

static void ref_drop_front(void) {
    for (int i = 1; i < ref_n; ++i) {
        ref[i - 1] = ref[i];
    }
    ref_n--;
}

static void apply(char op) {
    int v     = 0;
    int value = next_value++;
    switch (op) {
    case 't': // try_emplace
        if (CAP == 0 || ref_n == CAP) {
            assert(!try_emplace(value)); // a full buffer refuses and changes nothing
        } else {
            assert(try_emplace(value));
            ref[ref_n++] = value;
            pushes++;
        }
        break;
    case 'p': // push(const T&)
    case 'm': // push(T&&)
    case 'e': // emplace
        if (op == 'p') {
            push(value);
        } else if (op == 'm') {
            push_mv(value);
        } else {
            emplace(value);
        }
        if (CAP > 0) {
            if (ref_n == CAP) { // overwrite on full: the oldest goes
                ref_drop_front();
                evicted++;
            }
            ref[ref_n++] = value;
            pushes++;
        }
        break;
    case 'o': // pop
        if (ref_n == 0) {
            assert(!pop());
        } else {
            assert(pop());
            ref_drop_front();
            pops++;
        }
        break;
    case 'q': // top_pop
        if (ref_n == 0) {
            assert(!top_pop(&v));
        } else {
            assert(top_pop(&v) && v == ref[0]);
            ref_drop_front();
            pops++;
        }
        break;
    case 'c': // consume
        if (ref_n == 0) {
            assert(!consume(&v));
        } else {
            assert(consume(&v) && v == ref[0]);
            ref_drop_front();
            pops++;
        }
        break;
    default: // clear
        pops += (unsigned long)ref_n;
        clear();
        ref_n = 0;
        break;
    }
    check_state();
}

static void reset_all(void) {
    set_up();
    atomic_store_explicit(&g_evictions, 0UL, memory_order_relaxed);
    ref_n = 0;
    next_value = 1;
    pushes = pops = evicted = 0;
}

// the container's own destructor: it destroys every element it holds
static void teardown(void) {
    for (unsigned long i = 0; i < SLOTS; ++i) {
        destroy(i);
    }
}

int main(void) {
#if OBS == 0
    reset_all();
    check_state();
    const char *script = STR(SCRIPT);
    for (int i = 0; script[i] != 0; ++i) {
        apply(script[i]);
    }
    // whatever is left comes out in order
    int v = 0;
    for (int i = 0; i < ref_n; ++i) {
        assert(top_pop(&v) && v == ref[i]);
    }
    assert(!top_pop(&v) && is_empty());
#elif OBS == 1
    reset_all();
    push(1);
    assert(pop());
    push_mv(2); // operator= on the object the destructor ran on
#elif OBS == 2
    reset_all();
    push(1);
    assert(pop());
    teardown(); // the vector or array destroys all its elements, the popped one a second time
#elif OBS == 3
    reset_all();
    push(1);
    assert(pop());
    emplace(2); // placement new: fine
    teardown();
#else
    reset_all();
    for (int i = 0; i < CAP; ++i) {
        push(1 + i);
    }
    push_mv(9); // full: the oldest is destroyed, its slot is assigned
#endif
    return 0;
}

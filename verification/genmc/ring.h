// GenMC model of the concurrent logic of include/CircularBuffer.hpp, transcribed line by line.
//
// Header checked: see HEADER_SHA256 in run.py. Every model statement that stands for a header line carries
//     // hpp:<line> "<text that must occur on that header line>"
// and every memory order of the header's concurrent functions has a SITE marker
//     /* SITE <macro> hpp:<line> <order> */
// followed by its default definition. run.py (arm "fidelity") checks both mechanically.
//
// One model serves the three storage forms (std::vector<T>, std::array<T, N>, std::span<T>): they share every
// atomic line; only increment(), full(), get_capacity() and get_last() choose N or m_max_size, with the same
// formula. CAP is that number. T is int; the slots are PLAIN memory so GenMC reports any data race on a slot.
//
// Configuration (all -D):
//   CAP            capacity, 0..3                                        (default 2)
//   DTOR_WRITES    1: the destructor of T writes DESTROYED into the slot, as the destructor of a non-trivial T
//                  writes memory (default 1, the conservative reading); 0: T is trivially destructible
//   GHOST          1: count evictions, check the cause of every CAS retry
//   SPURIOUS       n: the first n weak compare-and-swaps of the run fail spuriously (default 0)
//   O_<SITE>       override one memory order (mutants and the fix variants)
//   STORAGE_ARRAY  0: vector / span lines of full() (default); 1: the std::array line
//   M_*            structural mutants, each named where it acts
#ifndef RING_H
#define RING_H

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
#ifndef SPURIOUS
#define SPURIOUS 0
#endif

#define DESTROYED (-1)
#define UNWRITTEN 0

#if CAP > 0
#define SLOTS CAP
#else
#define SLOTS 1
#endif

// ---------------------------------------------------------------------------------------------------------
// Memory orders of the header, one site each. A mutant redefines the macro with -D.
// ---------------------------------------------------------------------------------------------------------
/* SITE O_TAIL_LOAD        hpp:1277 relaxed */
#ifndef O_TAIL_LOAD
#define O_TAIL_LOAD memory_order_relaxed
#endif
/* SITE O_TAIL_CAS_OK      hpp:1285 release */
#ifndef O_TAIL_CAS_OK
#define O_TAIL_CAS_OK memory_order_release
#endif
/* SITE O_TAIL_CAS_FAIL    hpp:1285 relaxed */
#ifndef O_TAIL_CAS_FAIL
#define O_TAIL_CAS_FAIL memory_order_relaxed
#endif
/* SITE O_HEAD_LOAD_RI     hpp:1297 acquire */
#ifndef O_HEAD_LOAD_RI
#define O_HEAD_LOAD_RI memory_order_acquire
#endif
/* SITE O_HEAD_CAS_RI_OK   hpp:1308 release */
#ifndef O_HEAD_CAS_RI_OK
#define O_HEAD_CAS_RI_OK memory_order_release
#endif
/* SITE O_HEAD_CAS_RI_FAIL hpp:1309 relaxed */
#ifndef O_HEAD_CAS_RI_FAIL
#define O_HEAD_CAS_RI_FAIL memory_order_relaxed
#endif
/* SITE O_COUNT_ADD_COPY   hpp:1327 release */
#ifndef O_COUNT_ADD_COPY
#define O_COUNT_ADD_COPY memory_order_release
#endif
/* SITE O_COUNT_ADD_MOVE   hpp:1347 release */
#ifndef O_COUNT_ADD_MOVE
#define O_COUNT_ADD_MOVE memory_order_release
#endif
/* SITE O_COUNT_ADD_EMPL   hpp:1368 release */
#ifndef O_COUNT_ADD_EMPL
#define O_COUNT_ADD_EMPL memory_order_release
#endif
/* SITE O_HEAD_LOAD_TOP    hpp:1378 acquire */
#ifndef O_HEAD_LOAD_TOP
#define O_HEAD_LOAD_TOP memory_order_acquire
#endif
/* SITE O_TAIL_LOAD_LAST   hpp:1388 acquire */
#ifndef O_TAIL_LOAD_LAST
#define O_TAIL_LOAD_LAST memory_order_acquire
#endif
/* SITE O_COUNT_SUB_TOPPOP hpp:1419 relaxed */
#ifndef O_COUNT_SUB_TOPPOP
#define O_COUNT_SUB_TOPPOP memory_order_relaxed
#endif
/* SITE O_COUNT_SUB_POP    hpp:1441 acq_rel */
#ifndef O_COUNT_SUB_POP
#define O_COUNT_SUB_POP memory_order_acq_rel
#endif
/* SITE O_COUNT_LOAD_EMPTY hpp:1449 acquire */
#ifndef O_COUNT_LOAD_EMPTY
#define O_COUNT_LOAD_EMPTY memory_order_acquire
#endif
/* SITE O_COUNT_LOAD_FULL  hpp:1455 acquire */
#ifndef O_COUNT_LOAD_FULL
#define O_COUNT_LOAD_FULL memory_order_acquire
#endif
/* SITE O_COUNT_LOAD_FULLN hpp:1457 acquire */
#ifndef O_COUNT_LOAD_FULLN
#define O_COUNT_LOAD_FULLN memory_order_acquire
#endif
/* SITE O_COUNT_LOAD_SIZE  hpp:1463 acquire */
#ifndef O_COUNT_LOAD_SIZE
#define O_COUNT_LOAD_SIZE memory_order_acquire
#endif
/* SITE O_HEAD_LOAD_CONS   hpp:734 acquire */
#ifndef O_HEAD_LOAD_CONS
#define O_HEAD_LOAD_CONS memory_order_acquire
#endif
/* SITE O_HEAD_LOAD_EV     hpp:1762 acquire */
#ifndef O_HEAD_LOAD_EV
#define O_HEAD_LOAD_EV memory_order_acquire
#endif
/* SITE O_HEAD_CAS_EV_OK   hpp:1767 release */
#ifndef O_HEAD_CAS_EV_OK
#define O_HEAD_CAS_EV_OK memory_order_release
#endif
/* SITE O_HEAD_CAS_EV_FAIL hpp:1768 relaxed */
#ifndef O_HEAD_CAS_EV_FAIL
#define O_HEAD_CAS_EV_FAIL memory_order_relaxed
#endif
/* SITE O_COUNT_SUB_EV     hpp:1780 acq_rel */
#ifndef O_COUNT_SUB_EV
#define O_COUNT_SUB_EV memory_order_acq_rel
#endif

// ---------------------------------------------------------------------------------------------------------
// The state: m_head, m_tail, m_count atomic; the slots plain.
// ---------------------------------------------------------------------------------------------------------
static atomic_ulong m_head;  // hpp:1752 "std::atomic<size_type> m_head, m_tail, m_count;"
static atomic_ulong m_tail;  // hpp:1752 "m_head, m_tail, m_count"
static atomic_ulong m_count; // hpp:1752 "m_count;"
#ifndef SLOTS_ATOMIC
#define SLOTS_ATOMIC 0 // 1: the elements are std::atomic<T> (seq_cst), as in std::vector<std::atomic<T>>
#endif
#if SLOTS_ATOMIC
static atomic_int slot[SLOTS]; // atomic elements: no data race is possible on a slot
#define SLOT_RD(i)    atomic_load_explicit(&slot[(i)], memory_order_seq_cst)
#define SLOT_WR(i, v) atomic_store_explicit(&slot[(i)], (v), memory_order_seq_cst)
#else
static int slot[SLOTS]; // the container's elements: plain, so a race on one is reported
#define SLOT_RD(i)    slot[(i)]
#define SLOT_WR(i, v) (slot[(i)] = (v))
#endif
#ifndef LIFETIME
#define LIFETIME 0 // bit 0: an assignment into a destroyed object fails; bit 1: a second destructor on one object fails
#endif
static int life[SLOTS]; // 1: a live object; 0: destroyed (vector(size) and array construct every element)

// ghost state: not in the header, never read by the ring's logic
static atomic_ulong g_evictions;
static atomic_int   g_spurious_left = SPURIOUS;
static atomic_ulong g_retries; // CAS retries, all threads

// positions seen when no thread is running (the end of a scenario)
static unsigned long head_pos(void)  { return atomic_load_explicit(&m_head, memory_order_relaxed); }
static unsigned long tail_pos(void)  { return atomic_load_explicit(&m_tail, memory_order_relaxed); }
static unsigned long count_now(void) { return atomic_load_explicit(&m_count, memory_order_relaxed); }

static int set_up(void) {
    for (unsigned long i = 0; i < SLOTS; ++i) {
        SLOT_WR(i, UNWRITTEN); // the element exists (vector(size) / array / the caller's object) and holds nothing yet
#if LIFETIME
        life[i] = 1;
#endif
    }
    atomic_init(&m_head, 0UL);  // hpp:118 "m_head(0UL)"
    atomic_init(&m_tail, 0UL);  // hpp:118 "m_tail(0UL)"
    atomic_init(&m_count, 0UL); // hpp:118 "m_count(0UL)"
    return 0;
}

// A weak compare-and-swap, as the header's compare_exchange_weak. With SPURIOUS the first n calls of the run
// fail without storing and return the value they loaded in *expected, as a failed LL/SC does. With GHOST a
// failure is checked: it must come with a changed value (another thread's completed claim) unless spurious,
// and the retries of one call are bounded by RETRY_BOUND (the number of other claims that can happen).
#ifndef RETRY_BOUND
#define RETRY_BOUND 4
#endif
static int weak_cas(atomic_ulong *obj, unsigned long *expected, unsigned long desired, memory_order ok,
                    memory_order fail, unsigned *retries) {
    const unsigned long was = *expected;
    int                 succeeded;
#if SPURIOUS > 0
    if (atomic_fetch_sub_explicit(&g_spurious_left, 1, memory_order_relaxed) > 0) {
        *expected = atomic_load_explicit(obj, fail);
        succeeded = 0;
    } else
#endif
    {
        succeeded = atomic_compare_exchange_weak_explicit(obj, expected, desired, ok, fail);
    }
    if (succeeded) {
        return 1;
    }
#if GHOST
    assert(*expected != was || SPURIOUS > 0); // a retry has a cause: the word was moved by a completed claim
    if (retries != NULL) {
        *retries += 1;
        assert(*retries <= RETRY_BOUND);
        atomic_fetch_add_explicit(&g_retries, 1UL, memory_order_relaxed);
    }
#else
    (void)was;
    (void)retries;
#endif
    return 0;
}

// The destructor of T on a slot.
static void destroy(unsigned long index) {
#if LIFETIME & 2
    assert(life[index] == 1); // a second destructor on an object already destroyed
#endif
#if LIFETIME
    life[index] = 0;
#endif
#if DTOR_WRITES
    SLOT_WR(index, DESTROYED); // a non-trivial ~T() writes the object
#else
    (void)index; // a trivial ~T() writes nothing
#endif
}

// ---------------------------------------------------------------------------------------------------------
// size_type increment(const size_type& value) const  hpp:1499
// ---------------------------------------------------------------------------------------------------------
static unsigned long increment(unsigned long value) { // hpp:1499 "size_type increment(const size_type& value) const {"
    // hpp:1500 "if constexpr((N == 0 || N == std::dynamic_extent)) {"
#ifdef M_BAD_WRAP
    return (value + 1) % (CAP + 1); // mutant: the index wraps one slot late
#else
    return (value + 1) % (CAP == 0 ? 1 : CAP); // hpp:1501 "(value + 1) % (m_max_size == 0 ? 1 : m_max_size)"; N form hpp:1503 "(value + 1) % N"
#endif
}

static int is_empty(void) { // hpp:1447
    return atomic_load_explicit(&m_count, O_COUNT_LOAD_EMPTY) == 0; // hpp:1449 "m_count.load(std::memory_order_acquire) == 0"
}

#ifndef STORAGE_ARRAY
#define STORAGE_ARRAY 0 // 0: std::vector / std::span (compares with m_max_size); 1: std::array (compares with N)
#endif
static int full(void) { // hpp:1453 "bool full(void) const {"
    // hpp:1454 "if constexpr ((N == 0 || N == std::dynamic_extent)) {"
#if STORAGE_ARRAY
    return atomic_load_explicit(&m_count, O_COUNT_LOAD_FULLN) == CAP; // hpp:1457 "return m_count.load(std::memory_order_acquire) == N;"
#else
    return atomic_load_explicit(&m_count, O_COUNT_LOAD_FULL) == CAP; // hpp:1455 "return m_count.load(std::memory_order_acquire) == m_max_size;"
#endif
}

static unsigned long get_size(void) { // hpp:1461
    return atomic_load_explicit(&m_count, O_COUNT_LOAD_SIZE); // hpp:1463 "return m_count.load(std::memory_order_acquire);"
}

static unsigned long get_capacity(void) { // hpp:1467 "size_type get_capacity(void) const {"
    // hpp:1469 "if constexpr ((N > 0 && N != std::dynamic_extent)) {"
    return CAP; // hpp:1470 "return N;" / hpp:1472 "return m_max_size;"
}

// ---------------------------------------------------------------------------------------------------------
// void pop_front_unsafe(void)  hpp:1756 : evict the oldest item, with no check that there still is one
// ---------------------------------------------------------------------------------------------------------
static void pop_front_unsafe(void) {
    unsigned long current_head = 0, next_head = 0; // hpp:1758 "size_type current_head{0}, next_head{0};"
#ifdef M_NO_RETRY_EV
    current_head = atomic_load_explicit(&m_head, O_HEAD_LOAD_EV);
    next_head    = increment(current_head);
    weak_cas(&m_head, &current_head, next_head, O_HEAD_CAS_EV_OK, O_HEAD_CAS_EV_FAIL, NULL); // mutant: no retry
#elif defined(M_HEAD_STORE_EV)
    current_head = atomic_load_explicit(&m_head, O_HEAD_LOAD_EV);
    next_head    = increment(current_head);
    atomic_store_explicit(&m_head, next_head, O_HEAD_CAS_EV_OK); // mutant: the compare-and-swap is a plain store
#else
    unsigned retries = 0; // ghost: retries of this call
    do { // hpp:1760 "do {"
        current_head = atomic_load_explicit(&m_head, O_HEAD_LOAD_EV); // hpp:1762 "current_head = m_head.load(std::memory_order_acquire);"
        next_head    = increment(current_head);                       // hpp:1764 "next_head = increment(current_head);"
    } while (!weak_cas(&m_head, &current_head, next_head, O_HEAD_CAS_EV_OK, O_HEAD_CAS_EV_FAIL, &retries)); // hpp:1766 "m_head.compare_exchange_weak(current_head, next_head,"
#endif
#ifdef M_SUB_BEFORE_DESTROY_EV
    atomic_fetch_sub_explicit(&m_count, 1UL, O_COUNT_SUB_EV);
    destroy(current_head);
#else
    destroy(current_head);                                            // hpp:1776 "this->operator[](current_head).~T();"
    atomic_fetch_sub_explicit(&m_count, 1UL, O_COUNT_SUB_EV);         // hpp:1780 "m_count.fetch_sub(1, std::memory_order_acq_rel);"
#endif
#if GHOST
    atomic_fetch_add_explicit(&g_evictions, 1UL, memory_order_relaxed);
#endif
}

// ---------------------------------------------------------------------------------------------------------
// size_type insert(void)  hpp:1272 : claim the slot at the tail, evicting first when the buffer looks full
// ---------------------------------------------------------------------------------------------------------
static unsigned long insert(void) {
    unsigned long current_tail = 0, next_tail = 0; // hpp:1274 "size_type current_tail{0UL}, next_tail{0UL};"
    unsigned retries = 0; // ghost: retries of this call
#ifdef M_NO_RETRY_TAIL
    current_tail = atomic_load_explicit(&m_tail, O_TAIL_LOAD);
    next_tail    = increment(current_tail);
    if (full()) {
        pop_front_unsafe();
    }
    weak_cas(&m_tail, &current_tail, next_tail, O_TAIL_CAS_OK, O_TAIL_CAS_FAIL, NULL); // mutant: a failed claim is ignored
#elif defined(M_TAIL_STORE)
    current_tail = atomic_load_explicit(&m_tail, O_TAIL_LOAD);
    next_tail    = increment(current_tail);
    if (full()) {
        pop_front_unsafe();
    }
    atomic_store_explicit(&m_tail, next_tail, O_TAIL_CAS_OK); // mutant: the compare-and-swap is a plain store
#else
#ifdef M_NO_FULL_RECHECK
    int first = 1;
#endif
    do { // hpp:1276 "do {"
        current_tail = atomic_load_explicit(&m_tail, O_TAIL_LOAD); // hpp:1277 "current_tail = m_tail.load(std::memory_order_relaxed);"
        next_tail    = increment(current_tail);                    // hpp:1279 "next_tail = increment(current_tail);"
#ifdef M_NO_FULL_RECHECK
        if (first && full()) { // mutant: the room is checked on the first pass only
            pop_front_unsafe();
        }
        first = 0;
#else
        if (full()) { // hpp:1281 "if (full()) {"
            pop_front_unsafe(); // hpp:1282 "pop_front_unsafe();"
        }
#endif
    } while (!weak_cas(&m_tail, &current_tail, next_tail, O_TAIL_CAS_OK, O_TAIL_CAS_FAIL, &retries)); // hpp:1285 "m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed)"
#endif
    return current_tail; // hpp:1287 "return current_tail;"
}

// ---------------------------------------------------------------------------------------------------------
// std::optional<size_type> read_insert(void)  hpp:1291 : claim the oldest item, or report empty
// ---------------------------------------------------------------------------------------------------------
static int read_insert(unsigned long *out) {
    unsigned long current_head = 0, next_head = 0; // hpp:1293 "size_type current_head{0}, next_head{0};"
    unsigned retries = 0; // ghost: retries of this call
#ifdef M_NO_RETRY_RI
    current_head = atomic_load_explicit(&m_head, O_HEAD_LOAD_RI);
    if (is_empty()) {
        return 0;
    }
    next_head = increment(current_head);
    weak_cas(&m_head, &current_head, next_head, O_HEAD_CAS_RI_OK, O_HEAD_CAS_RI_FAIL, NULL); // mutant: a failed claim is ignored
#elif defined(M_HEAD_STORE_RI)
    current_head = atomic_load_explicit(&m_head, O_HEAD_LOAD_RI);
    if (is_empty()) {
        return 0;
    }
    next_head = increment(current_head);
    atomic_store_explicit(&m_head, next_head, O_HEAD_CAS_RI_OK); // mutant: the compare-and-swap is a plain store
#else
#ifdef M_NO_EMPTY_RECHECK
    int first = 1;
#endif
    do { // hpp:1295 "do {"
        current_head = atomic_load_explicit(&m_head, O_HEAD_LOAD_RI); // hpp:1297 "current_head = m_head.load(std::memory_order_acquire);"
#ifdef M_NO_EMPTY_RECHECK
        if (first && is_empty()) { // mutant: emptiness is checked on the first pass only
            return 0;
        }
        first = 0;
#else
        if (is_empty()) { // hpp:1299 "if (is_empty()) {"
#ifdef M_WAIT_EMPTY
            while (is_empty()) { // mutant: wait for an item instead of answering "empty": a blocking pop
            }
#else
            return 0;     // hpp:1301 "return std::nullopt;"
#endif
        }
#endif
        next_head = increment(current_head); // hpp:1305 "next_head = increment(current_head);"
    } while (!weak_cas(&m_head, &current_head, next_head, O_HEAD_CAS_RI_OK, O_HEAD_CAS_RI_FAIL, &retries)); // hpp:1307 "m_head.compare_exchange_weak(current_head, next_head,"
#endif
    *out = current_head; // hpp:1311 "return current_head;"
    return 1;
}

// ---------------------------------------------------------------------------------------------------------
// push_back, emplace_back  hpp:1315 / 1331 / 1351 : claim a slot, write it, count it
// ---------------------------------------------------------------------------------------------------------
static void push_back_copy(int item) { // hpp:1315 "void push_back(const T& item) {"
    if (get_capacity() == 0UL) { // hpp:1317 "if (get_capacity() == 0UL) {"
        return;                  // hpp:1318 "return;"
    }
#ifdef M_COUNT_BEFORE_WRITE
    atomic_fetch_add_explicit(&m_count, 1UL, O_COUNT_ADD_COPY);
    SLOT_WR(insert(), item);
#else
    const unsigned long assigned = insert();
#if LIFETIME & 1
    assert(life[assigned] == 1); // operator= on a destroyed object
#endif
    SLOT_WR(assigned, item); // hpp:1321 "this->operator[](insert()) = item;"
    atomic_fetch_add_explicit(&m_count, 1UL, O_COUNT_ADD_COPY); // hpp:1327 "m_count.fetch_add(1, std::memory_order_release);"
#endif
}

static void push_back_move(int item) { // hpp:1331 "void push_back(T&& item) {"
    if (get_capacity() == 0UL) { // hpp:1333 "if (get_capacity() == 0UL) {"
        return;                  // hpp:1334 "return;"
    }
    const unsigned long current_tail = insert(); // hpp:1337 "const size_type current_tail = insert();"
#ifdef M_COUNT_BEFORE_WRITE
    atomic_fetch_add_explicit(&m_count, 1UL, O_COUNT_ADD_MOVE);
    SLOT_WR(current_tail, item);
#else
#if LIFETIME & 1
    assert(life[current_tail] == 1); // operator= on a destroyed object
#endif
    SLOT_WR(current_tail, item); // hpp:1339 "this->operator[](current_tail) = std::move(item);"
    atomic_fetch_add_explicit(&m_count, 1UL, O_COUNT_ADD_MOVE); // hpp:1347 "m_count.fetch_add(1, std::memory_order_release);"
#endif
}

static void emplace_back(int arg) { // hpp:1352 "void emplace_back(Args&&... args) {"
    if (get_capacity() == 0UL) { // hpp:1354 "if (get_capacity() == 0UL) {"
        return;                  // hpp:1355 "return;"
    }
    const unsigned long current_tail = insert(); // hpp:1358 "const size_type current_tail = insert();"
#ifdef M_COUNT_BEFORE_WRITE
    atomic_fetch_add_explicit(&m_count, 1UL, O_COUNT_ADD_EMPL);
    SLOT_WR(current_tail, arg);
#else
#if LIFETIME
    life[current_tail] = 1; // placement new makes a live object
#endif
    SLOT_WR(current_tail, arg); // hpp:1360 "new (&this->operator[](current_tail)) T(std::forward<Args>(args)...);"
    atomic_fetch_add_explicit(&m_count, 1UL, O_COUNT_ADD_EMPL); // hpp:1368 "m_count.fetch_add(1, std::memory_order_release);"
#endif
}

// ---------------------------------------------------------------------------------------------------------
// public API, as the wrappers of the header
// ---------------------------------------------------------------------------------------------------------
static void push(int item)    { push_back_copy(item); }  // hpp:498 "push_back(item);"
static void push_mv(int item) { push_back_move(item); } // hpp:526 "push_back(std::move(item));"
static void emplace(int arg)  { emplace_back(arg); }    // hpp:554 "emplace_back(std::forward<Args>(args)...);"

static int try_emplace(int arg) { // hpp:693
    if (full()) {                 // hpp:696 "if (full()) {"
        return 0;                 // hpp:697 "return false;"
    }
    emplace_back(arg); // hpp:700 "emplace_back(std::forward<Args>(args)...);"
    return 1;          // hpp:702 "return true;"
}

static int pop(void) { // hpp:581 "bool pop(void) {" -> pop_front hpp:1425
    unsigned long current_head = 0;
    if (!read_insert(&current_head)) { // hpp:1427 "read_insert();"  hpp:1429 "if (!current_head) {"
        return 0;                       // hpp:1430 "return false;"
    }
#ifdef M_SUB_BEFORE_DESTROY_POP
    atomic_fetch_sub_explicit(&m_count, 1UL, O_COUNT_SUB_POP);
    destroy(current_head);
#else
    destroy(current_head);                                    // hpp:1439 "this->operator[](current_head.value()).~T();"
    atomic_fetch_sub_explicit(&m_count, 1UL, O_COUNT_SUB_POP); // hpp:1441 "m_count.fetch_sub(1, std::memory_order_acq_rel);"
#endif
    return 1; // hpp:1443 "return true;"
}

static int top_pop(int *out) { // hpp:669 -> get_top_pop hpp:1401
    unsigned long current_head = 0;
    if (!read_insert(&current_head)) { // hpp:1403 "read_insert();"  hpp:1405 "if (!current_head) {"
        return 0;                       // hpp:1406 "return std::nullopt;"
    }
#ifdef M_SUB_BEFORE_READ_TOPPOP
    atomic_fetch_sub_explicit(&m_count, 1UL, O_COUNT_SUB_TOPPOP);
    int value = SLOT_RD(current_head);
    destroy(current_head);
#else
    int value = SLOT_RD(current_head); // hpp:1409 "T value = this->operator[](current_head.value());"
    destroy(current_head);          // hpp:1411 "this->operator[](current_head.value()).~T();"
    atomic_fetch_sub_explicit(&m_count, 1UL, O_COUNT_SUB_TOPPOP); // hpp:1419 "m_count.fetch_sub(1, std::memory_order_relaxed);"
#endif
    *out = value; // hpp:1421 "return value;"
    return 1;
}

static int top(int *out) { // hpp:608 -> get_top hpp:1372
    if (is_empty()) {      // hpp:1374 "if (is_empty()) {"
        return 0;          // hpp:1375 "return std::nullopt;"
    }
    *out = SLOT_RD(atomic_load_explicit(&m_head, O_HEAD_LOAD_TOP)); // hpp:1378 "return this->operator[](m_head.load(std::memory_order_acquire));"
    return 1;
}

static int last(int *out) { // hpp:635 -> get_last hpp:1382
    if (is_empty()) {       // hpp:1384 "if (is_empty()) {"
        return 0;           // hpp:1385 "return std::nullopt;"
    }
    // hpp:1389 "if constexpr((N > 0 && N != std::dynamic_extent)) {"  
    // hpp:1393 "if constexpr((N == 0 || N == std::dynamic_extent)) {"  hpp:1394 "(_tail_position == 0) ? (m_max_size - 1) : (_tail_position - 1);"  (the vector / span form: same formula, m_max_size for N)
    unsigned long tail_position = atomic_load_explicit(&m_tail, O_TAIL_LOAD_LAST); // hpp:1388 "size_type _tail_position = m_tail.load(std::memory_order_acquire);"
    tail_position = (tail_position == 0) ? (CAP - 1) : (tail_position - 1);       // hpp:1390 "_tail_position = (_tail_position == 0) ? (N - 1) : (_tail_position - 1);"
    *out = SLOT_RD(tail_position); // hpp:1397 "return this->operator[](_tail_position);"
    return 1;
}

// consume(fn): fn reads the oldest item where it sits; here fn stores what it read into *out.
static int consume(int *out) { // hpp:727
    if (is_empty()) {          // hpp:730 "if (is_empty()) {"
        return 0;              // hpp:731 "return false;"
    }
    *out = SLOT_RD(atomic_load_explicit(&m_head, O_HEAD_LOAD_CONS)); // hpp:734 "std::forward<Fn>(fn)(std::as_const(this->operator[](m_head.load(std::memory_order_acquire))));"
    return pop();                                                  // hpp:736 "return pop_front();"
}

static void clear(void) { // hpp:1487
    while (!is_empty()) { // hpp:1489 "while (!is_empty()) {"
        if (!pop()) {     // hpp:1491 "if(!pop_front()){"
            break;        // hpp:1492 "break;"
        }
    }
}

#endif // RING_H

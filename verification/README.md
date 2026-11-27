# Formal verification of CircularBuffer

This folder holds a GenMC model of the ring's concurrent logic and a runner that checks it. GenMC explores EVERY
execution of a small scenario under the C11 memory model (RC11), reporting any data race on a slot, any failed
assertion and, with `--check-liveness`, any thread that can spin for ever.

**What it proves is bounded.** A passing arm is a proof for that scenario only (capacity 1 to 3, two or three
threads, two to five operations per thread, `int` elements). A failing arm is a real counter-example: GenMC's trace is
an execution the C++ memory model allows. The models transcribe the header line by line (see the correspondence
table at the end, checked mechanically by `run.py`), so a failure is a failure of the header, not of the model.

Header checked: `include/CircularBuffer.hpp`, sha256 `bc408e38ccb1f33a73a98c8009a8abccdd2735e621d3126be4288a04553725d8`
(the working tree of 2026-11-26, HEAD f5faa2b plus the uncommitted caller-storage edit).

## How to run

    python3 verification/genmc/run.py              # every arm, one line each; exit code 1 on any mismatch
    python3 verification/genmc/run.py --list       # the arms
    python3 verification/genmc/run.py --only spsc  # arms whose name contains "spsc" (repeatable)
    python3 verification/genmc/run.py --table      # the line-by-line correspondence table
    python3 verification/genmc/run.py -v --only mc2-prefill1   # GenMC's trace of one arm

Needs GenMC (tested with v0.11.0, LLVM 19) and `clang` (for its header directory) on the path; Python 3 standard
library only. One GenMC process runs at a time, at `nice 19`; options `--timeout` (600 s per arm), `--max-rss-mb`
(1000), `--min-avail-mb` (1500) stop an arm that grows too large. Output: `arm | expected | actual | executions | seconds`
("+Nb" counts blocked executions: spin-loop iterations GenMC prunes).

## What "lock-free" means here

An operation is lock-free if some thread always completes an operation in a bounded number of the system's steps,
whatever the others do (no thread can be made to wait on another that was descheduled); wait-free if every thread
does, obstruction-free if a thread that runs alone completes. The header contains no blocking primitive (the arm
`text-search` lists every hit of mutex, lock, condition variable, wait, sleep, yield, semaphore, futex, spin, thread:
no hit is code), and every loop is a compare-and-swap retry
or bounded by the item count.

**What is checked and what is argued.**

| claim | how |
|---|---|
| no blocking primitive anywhere in the header | **checked** by `text-search`: 24 hits of the pattern, none in code: 12 are the word "thread" in comments, 3 are "Lock-free" / "block" in comments, 9 are the macro name `CIRCULARBUFFER_MULTI_THREADING` (the TBB switch); mutex, condition variable, wait, sleep, yield, semaphore, futex, spin, barrier, latch: no hit at all |
| every loop is a compare-and-swap retry (`insert` 1276-1285, `read_insert` 1295-1309, `pop_front_unsafe` 1760-1768, `atomic_float_add/sub` 1705-1720) or bounded by the item count (`clear` 1489) | **read** |
| a retry happens only because another thread's compare-and-swap on the same word completed (so some thread made progress), or the hardware failed spuriously | **checked** in the models: after every failed compare-and-swap an assertion requires the word to have changed (`live-mp2-push-cap2`, `live-mc2-prefill2-cap2`); `retry-bound-0-mp2-push-cap2` shows the assertion does fire |
| a spurious `compare_exchange_weak` failure costs one retry, no more | **checked**: `live-spurious-*` (the first n weak compare-and-swaps of the run fail without storing), retries asserted `<=` the bound |
| no execution in which a thread spins for ever (GenMC `--check-liveness`) | **checked** on the header model for 1 producer + 1 consumer, 2 producers, 2 consumers (`live-*`); the control `live-mutant-wait-empty` (a `pop` that waits for an item) IS reported as a liveness violation, so the check bites |
| `try_emplace`, `consume`, `pop`, `top_pop`, `size`, `empty`, `full`, `capacity`, `top`, `last` are wait-free for one producer and one consumer (modulo spurious compare-and-swap failure) | **argued** from the loops: with one thread on the tail and one on the head no compare-and-swap can fail except spuriously; the 1P1C arms pass with bounded retries |
| `push`, `emplace`, `pop`, `top_pop` with several threads are lock-free (some thread always completes a step), not wait-free (a thread can lose every compare-and-swap) | **argued** from the retry structure, consistent with the live arms |
| `clear()` / `reset()` terminates | **only if the count is sane**: it loops `while (!is_empty())`, and after the count has wrapped (cause C2 below) `is_empty()` stays false for about 2^64 pops: argued, not modelled |
| the statistics' `atomic_float_add` loops are lock-free | argued; not modelled |

Lock-free says nothing about correctness: the same lock-free retry loops return wrong answers with several producers or
consumers (below). Liveness and safety are separate results.

## The contract, as the text gives it

* Class comment (hpp:42-47): "try_emplace() and consume() are for ONE producer thread and ONE consumer thread ...
  **Every other function keeps its meaning, many producers and many consumers included.**" Move and copy only while
  no other thread uses the buffers.
* `try_emplace` (hpp:678): one producer, at most one consumer. `consume` (hpp:711): one consumer.
* README: "Lock-Free Implementation using std::array", "std::vector in a thread-safe manner", and for the span form
  "one producer thread and one consumer thread, as for any buffer" (README line 197).
* **They disagree.** The header promises many producers and many consumers for `push`, `emplace`, `pop`, `top`,
  `top_pop`, `last`, `size`, `empty`, `full`; the README says one of each for any buffer. The README is silent on
  overwrite-on-full under concurrency, on `last()` and `top()` from a second thread, on the statistics and on `reset()`.
* The statistics (`sum`, `mean`, `variance`, `minimum`, `maximum`, the sorted and median functions), `reset`
  (= `clear`), copy and move are single-threaded in practice (they read the plain slots with no synchronisation).

The three storage forms share every atomic line; only `increment()`, `full()`, `get_capacity()` and `get_last()` pick
`N` or `m_max_size` with the same formula. One model serves all three (`-DSTORAGE_ARRAY=1` selects the `N` line of
`full()`, hpp:1457).

## Why the multi-producer / multi-consumer tests fail: the causes

The library's failing tests are `MultipleProducersSingleConsumer` and `MultipleProducersMultipleConsumers`
(test/CircularBufferDynamicTest.cpp:487 and :574, the Fixed twins). Both push 1000 items from two producers into a
buffer of 100 (so `push` overwrites constantly), start the consumer(s) when the FIRST producer is done, loop
`while (consumed < 1000 && !buffer.empty()) pop()` and then expect `buffer.empty()`. In the second test the consumers
hold `mtx` for their whole loop, so they run one after the other, never together.

### Summary: four causes

| cause | what is wrong | smallest fix | affects |
|---|---|---|---|
| **C1** `top_pop` lowers the count with a relaxed `fetch_sub` | the producer can reuse the slot while `top_pop` still reads it | one word: `release` on hpp:1419 | even ONE producer and ONE consumer |
| **C2** the consumer's claim and the count are two steps | a consumer (or the evicting `push`) takes an item that is not there | none small: needs per-slot ownership (design D) | several consumers; `push` on a full buffer against a consumer |
| **C3** the producer's claim, write and count are three steps | two producers claim one slot; a consumer reads a slot claimed but not yet written; "full" is judged on a stale count | none small: per-slot ownership (design D) | several producers; `try_emplace` from two threads |
| **C4** the tests stop at the first `empty()` | a correct queue fails them too | change the tests | both failing tests |

C2 and C3 have one root: the buffer keeps `head`, `tail` and `count` as three independent words and no state per slot.
Every check ("is there an item?", "is there room?") reads `count`; every claim moves `head` or `tail`; the slot is
touched in between and `count` changes last. `count` therefore lags a claim in one direction (a consumer's head moves
before the count drops; a producer's tail moves before the count rises) and leads the slot writes in the other (the
count rises when the LATER producer finishes, though an earlier slot is not yet written).

Each trace below is GenMC's output for the arm named (run `run.py -v --only <arm>` to see it). `head`, `tail`, `count`
are the header's `m_head`, `m_tail`, `m_count` after the step; a slot is shown by its content.

### C1: `top_pop` releases the slot with a relaxed decrement (hpp:1419)

Arm `spsc-te-toppop-cap1` (also cap 2, and with a trivially destructible `T`): FAIL (race).
The header comment of `try_emplace` says "only the consumer lowers the count, so a buffer seen not full stays not full
until this call has written its slot", and it relies on the consumer's lowering of the count to hand the slot back.
`pop` and `consume` do that with `acq_rel` (hpp:1441), `top_pop` with `relaxed` (hpp:1419).

1. Producer `try_emplace(20)` (hpp:696-702, 1277-1285, 1360, 1368): `full()` reads count 0; tail CAS 0 to 0 (capacity 1); slot[0] := 20; count 0 to 1 (release). head 0, tail 0, count 1.
2. Consumer `top_pop` (hpp:1297-1307, 1409-1419): reads count 1 (acquire), head CAS 0 to 0; reads slot[0] (20); runs `~T()` on slot[0]; `count.fetch_sub(1, relaxed)`: count 0.
3. Producer `try_emplace(21)`: `full()` reads count 0, written by the consumer's RELAXED decrement: no happens-before edge from step 2's read and destructor; tail CAS; slot[0] := 21. This write is unordered with the read and the destructor of step 2: data race (undefined behaviour; for a non-trivial `T`, a write into an object being destroyed).

Test relation: not the library's two tests (they use `push`), but it fails the one-producer one-consumer promise for `top_pop`.
Fix (text), hpp:1419:

    old:  m_count.fetch_sub(1, std::memory_order_relaxed);
    new:  m_count.fetch_sub(1, std::memory_order_release);

Arms `spsc-te-toppop-cap1-fix`, `spsc-te-toppop-cap2-fix` PASS (212 and 928 executions), and the arm set `fixE1-*` (every
non-mutant scenario of the table with this one change) changes no verdict except the `top_pop` arms, which turn PASS.
(`release` is enough: the consumer already acquired the count in `is_empty()`; `acq_rel`, as `pop_front` uses, also works.)
Hardware note: GenMC uses RC11, which forbids a load being reordered after a later store; ARM hardware allows it for
relaxed accesses, so on ARM the relaxed version can fail in more ways than GenMC shows.

### C2: a consumer claims an item that is not there

Arms `mc2-prefill1-cap2` (FAIL assert), `mc2-consume-prefill2-cap2` (FAIL race), `spmc-push-toppop-cap3`, and the
eviction arms `spsc-push-evict-*-cap1/2` (FAIL race): a thread that pushes on a full buffer evicts through
`pop_front_unsafe`, which is a consumer claim made on a stale `full()`.

Shortest trace, `mc2-prefill1-cap2` (capacity 2, ONE item in the buffer, two consumers each calling `top_pop` once):

1. Main pushed item 1: slot[0] = 1, head 0, tail 1, count 1.
2. Consumer A `top_pop`: `is_empty()` reads count 1; head CAS 0 to 1 (hpp:1307); reads slot[0] = 1; `~T()`; count 1 to 0 (hpp:1419).
3. Consumer B `top_pop`, started when A had moved the head but B still read the OLD count: loads head 1; `is_empty()` reads count 1 (stale, from step 1; hpp:1299); head CAS 1 to 0 (hpp:1307); reads slot[1] = nothing was ever written there (value 0): B "took" an item that does not exist; `~T()`; count 0 to **-1** (wraps to 2^64-1; hpp:1419). head 0, tail 1, count 2^64-1.

The two steps that are not atomic together: the emptiness check (`count`, hpp:1299) and the claim (`head` CAS, hpp:1307); the
count is lowered only after the claim and after the slot read (hpp:1419 / 1441). `empty()` is then false for ever
(count 2^64-1): `clear()` would run about 2^64 pops, and a test's final `buffer.empty()` fails.
`mc2-consume-prefill2-cap2` is the same defect with `consume`: both consumers read the same head slot (hpp:734) before either pops.
Eviction variant (`spsc-push-evict-pop-cap2`, `spsc-push-evict-toppop-cap1/2`; one producer calling `push` on a full buffer, one consumer calling `pop` or `top_pop`):
`insert` tests `full()` (hpp:1281, a read of the count) and then calls `pop_front_unsafe` (hpp:1282, 1756-1782), which moves the head and lowers the count
WITHOUT checking again that the buffer is still full or non-empty. The consumer's `pop` is a second claimant of the same head: GenMC's trace shows both
claiming a head slot after the head has wrapped (one destroys it while the other writes it: a data race), and, with the slots made atomic so that the race cannot
hide it, the model reports `size() > capacity` (the count passes the capacity or wraps), i.e. the ring's own range invariant breaks, with ONE producer and ONE consumer.

Tests: this cannot happen in `MultipleProducersMultipleConsumers` as written, because `consumer1` holds `mtx` for its
whole loop and `consumer2` only starts after it ends (the consumers never run together). The same claim-before-count window opens between the
EVICTING producer and the consumer in both tests (C2 eviction variant), which is what the models reproduce.

### C3: a producer's claim, write and count are three steps

Arms `mp2-push-cap1` (FAIL race), `mp2-try_emplace-cap1` (FAIL assert), `mp2-push-evict-cap2` (FAIL assert),
`mpsc-push-toppop-cap3` (FAIL race), `mptest-both-done-cap2` (FAIL race).

(a) Two producers claim one slot, `mp2-push-cap1` (capacity 1, two producers, one `push` each, no consumer):

1. Producer P1: `full()` reads count 0; tail CAS 0 to 0 (the one slot: tail wraps to itself, hpp:1285); slot[0] := 20 (hpp:1321). Count not yet raised (hpp:1327).
2. Producer P2: loads tail 0, `full()` reads count 0 (P1 has not counted yet, hpp:1281); tail CAS 0 to 0 succeeds again; slot[0] := 30.
   Two threads wrote slot[0] with no ordering: data race, one item lost (and with capacity above 1 a lap-late producer does the same).
   The count then reaches 2 with capacity 1 (assertion `count <= capacity` fails in the atomic-slot variant).

The room check (`full()`, count) and the slot claim (tail CAS) are different words; the count rises only after the write.
`mp2-try_emplace-cap1` is the same with `try_emplace` (documented one producer).

(b) The count is raised by the wrong producer, `mpsc-push-toppop-cap3` (capacity 3, two producers, one consumer, no eviction):

1. P1: tail CAS 0 to 1 (hpp:1285); has not written slot[0] yet.
2. P2: tail CAS 1 to 2; slot[1] := 30; count 0 to 1 (release, hpp:1327).
3. Consumer `top_pop`: `is_empty()` reads count 1 (acquire: this synchronises with P2 only, so it orders P2's write of slot[1], not P1's write of slot[0]); head CAS 0 to 1; reads slot[0]. P1 claimed that slot in step 1 and its write is not ordered before this read: data race; if P1 is slow the read returns nothing (value 0). head 1, tail 2, count 1.

`count` says "one item" but not which slot holds it: the later claimant publishes first. This breaks FIFO order and
reads uninitialised data for any `T`.

(c) A full buffer judged not full, `mp2-push-evict-cap2` (capacity 2, two producers, two `push` each, no consumer; GenMC's trace, 5 executions):

1. P1 `push(20)`: tail CAS 0 to 1; slot[0] := 20; count 0 to 1. head 0, tail 1, count 1.
2. P1 `push(21)`: `full()` reads count 1: not full (hpp:1281); tail CAS 1 to 0 (wrapped); slot[1] := 21; count 1 to 2 (hpp:1327).
3. P2 `push(30)`, which read the count at step 1's value (1) before step 2's increment: `full()` false; tail CAS 0 to 1: it claims slot 0, the slot of item 20, which is still unread and not evicted; slot[0] := 30; count 2 to **3**. head 0, tail 1, count 3 on a capacity of 2: the first item was overwritten without being evicted, and `size()` exceeds the capacity.

The other direction exists too: both producers read `full()` true and both evict (count 2 to 0, two items lost for two pushes). In `insert` the `full()` check and the eviction sit
INSIDE the retry loop (hpp:1276-1285), so a producer that loses the tail compare-and-swap evicts again on its next pass.

Tests: C3 is what two producers do to each other in both tests (500 pushes each into 100 slots). It corrupts values and is a
data race (undefined behaviour); with `size_t` elements it shows as lost, repeated or garbage values and the model finds no
interleaving where the END count is wrong after both producers finish: the count equals pushes - pops - evictions exactly
(arms `mptest-both-done-cap1-norace`, `-cap2-norace`, `-norace-accounting`, with the data-race detector off:
114104 and 50536 executions, all end with `empty()` true). So C3 alone does not make the final `empty()` false.

### C4: the tests' own end condition (this is what makes `buffer.empty()` fail about once in 20)

Both tests start the consumer when the FIRST producer reports done (`ready` is set by whichever producer finishes first)
and stop it at the first `buffer.empty()` it reads (`while (consumed < 1000 and !buffer.empty())`). If the second producer is
still pushing (descheduled, or simply slower), the consumer drains the buffer, reads `empty()`, leaves, and the second
producer then pushes into a buffer nobody empties: the final `EXPECT_TRUE(buffer.empty())` fails. Any correct queue
fails this test in that interleaving.

Arms `mptest-first-done-cap1` / `-cap2` (model of the test: two producers, the consumer waits for `ready` set by the first
to finish): FAIL on the assertion `is_empty()` after 3 and 6 executions (the failure is the test's, not the library's: no queue can pass it; a run of the same arm on design D did not finish within the 10-minute limit and was dropped). With the consumer started only after BOTH producers are done
(`WAIT_BOTH=1`), the end state is empty in every execution (race detector off), so the failure rate is the chance that the
consumer wakes while the second producer still runs, about 1 in 20 as the other agent measured.
Fix of the tests (text): make the consumer's stop condition "both producers have finished AND `buffer.empty()`":

    old:  while ((consumed_count < items_to_produce) and !buffer.empty()) { ... }
    new:  while (producers_done.load() < 2 or !buffer.empty()) { if (buffer.pop()) { ++consumed_count; } }   // producers_done is incremented after each producer's last push

(and in the multi-consumer test drop the `produced_count > consumed_count` gate, which stops consumption once evictions make
`consumed_count` lag; the final count of items received cannot equal 1000 whenever `push` overwrote.)

A second, rarer way the final check can fail comes from C2: if the consumer's loop ends because `consumed_count` reached 1000
while evictions had already taken items, the count has wrapped (claims exceed pushes) and `empty()` is false for ever. The
models show the wrap (C2 trace) but, at 2 pushes per producer, not this end state: it is a hypothesis, not a result.

### D: the smallest design that fixes C2 and C3 (model `ring_seq.h`, arms `design-*`)

There is no small edit of the header for C2 and C3: every cure that keeps `count` as the only publication either lets a
producer reuse a slot a consumer still reads, or makes a thread wait for another (not lock-free). The smallest design that
passes is one sequence number per slot (the bounded many-producer many-consumer queue of Vyukov):

* Slot i holds a word `seq[i]`: `2*t` = free for ticket t, `2*t + 1` = filled by ticket t, `2*(t + capacity)` = freed, next lap.
* A producer reads `seq[tail % cap]` (acquire); if it equals `2*tail`, it claims the ticket with ONE compare-and-swap on `tail`,
  writes the slot, and publishes with ONE release store of `2*tail + 1`. If it is lower, the buffer is full: `try_emplace` returns false;
  `push` takes the oldest item out the way a consumer would (one `dequeue`) and retries, so it evicts only an item that is really there.
* A consumer reads `seq[head % cap]`; if it equals `2*head + 1` it claims the ticket with one compare-and-swap on `head`, reads the
  slot, runs `~T()`, and frees it with ONE release store of `2*(head + cap)`.
* `count` disappears: `size = tail - head` (an estimate while threads run). `top()` and `last()` check the published number first.

The factor 2 is needed for capacity 1 (the usual queue needs capacity >= 2).

**What the design costs in progress (read this before adopting it).** `try_emplace`, `pop`, `top_pop`, `consume` never wait: when the number says "not yet", they answer "full" or
"empty" and return. A producer that stalls between claiming a ticket and publishing it makes later items invisible to consumers until it resumes (the consumers
report "empty", they do not wait). But `push` on a FULL buffer, with a consumer that has taken the oldest item and not yet freed its slot, must wait for that
consumer to finish reading one element: it cannot overwrite a slot that is being read in place, and it cannot take the item out (the consumer already did). So
overwrite-on-full against a running consumer is lock-free on the header only because it ignores that consumer (cause C2); on the design it waits, bounded by the consumer's own
read of one element but not by progress of the system if that consumer is descheduled. GenMC shows it: the three scenarios `spsc-push-evict-*` on the design have infinite executions
(the pushing thread spins), and the run does not finish without its spin pass (10 minutes, cut); the arms `design-weak-*` run with the pass on (waiting executions
pruned, i.e. assuming the consumer finishes) and pass, as weaker evidence. A lock-free overwrite would need the consumer to read a COPY and validate it (a sequence lock), which
requires `T` trivially copyable. GenMC's `--check-liveness` cannot be used on the design: its spin detector blocks loops that index a slot by a loaded ticket
and reports "non-terminating spinloop" on executions that make progress (checked: with the pass off the same scenarios terminate).

Result: the same scenarios, the same checks (`design-*` arms, run with GenMC's spin-assume pass off so no execution is pruned): every arm that
fails on the header with 2 producers, 2 consumers, or `try_emplace`/`consume` from two threads PASSES on the design (overwrite-on-full against a running consumer: see the next paragraph);
`design-mptest-*` shows the two-producer test ends empty and race free; the design's own weakened orders are caught
(table in "Mutants"). Cost: per operation one acquire load of the slot's number, one compare-and-swap on the head or tail (the multi-thread
case) and one release store; the header does two read-modify-writes per operation (count and index) plus several acquire loads. Per slot one
`size_t` (8 bytes). What it does to the one-producer one-consumer path: with a single producer and single consumer the compare-and-swap
can be a plain store (nobody else moves the word), so the path costs one acquire load and one release store per side and NO read-modify-write:
its instruction count falls, it does not grow. The change can be confined to a policy parameter (single-thread path = today's logic, kept as is,
with fix C1; multi-thread path = design D), but it cannot be confined to the *code*: the numbers need storage, an `std::array<std::atomic<size_t>, N>` or a
vector beside the elements, and the caller-provided `std::span` form would need the caller to provide the numbers too (or the buffer to allocate them), which
conflicts with "allocates nothing". Not modelled: `top()` and `last()` from a thread that is neither producer nor consumer
(unsafe on both), non-trivial `T` lifetime (the destructor still runs on every pop and `operator=` still assigns into a destroyed
slot; placement `new` on every push would fix that, observation O1), more than 3 slots or 3 threads.

## Would std::atomic<T> slots fix it?

Tested: `-DSLOTS_ATOMIC=1 -DDTOR_WRITES=0` (every slot an `atomic_int` read and written `seq_cst`, as in
`std::vector<std::atomic<T>>` / `std::array<std::atomic<T>, N>`; nothing else changed; `T` trivially copyable, so `~T()` writes nothing). Arms `atomicslots-*`.

| failing arm of the header | with atomic slots | why |
|---|---|---|
| `spsc-te-toppop-cap1/2` (C1) | **fixed** | the data race on the slot is gone, and RC11 forbids the producer's later write from being read by the consumer's earlier load (no load-to-store reordering). On hardware that reorders a load after a later relaxed store (ARM) the wrong value could still be read: keep the release of C1 |
| `spsc-push-evict-toppop-cap1/2`, `spsc-push-evict-pop-cap2` (C2) | **still fails**: `size() <= capacity` violated | no race any more, but the evicting producer and the consumer still claim the same head and lower the count twice |
| `mp2-push-cap1`, `mp2-push-evict-cap2` (C3) | **still fails**: `size() <= capacity` violated | both producers still claim one slot (count 2 on capacity 1): one value is silently overwritten |
| `mp2-try_emplace-cap1` (C3) | **still fails**: `evictions == 0` violated | `try_emplace`, documented never to remove an item, evicted one |
| `mpsc-push-toppop-cap3` (C3b) | **still fails**: `value > 0` violated | the consumer takes the slot before its producer wrote it: it gets the initial value, not an item |
| `mc2-prefill1-cap2`, `spmc-push-toppop-cap3` (C2) | **still fails**: `value > 0` violated | the second consumer takes an empty slot; the count wraps |
| `mc2-consume-prefill2-cap2` (C2) | **still fails**: an item taken twice | both consumers read the same head slot |
| `spsc-te-last-consumer-cap2`, `spsc-te-top-producer-cap2` | **still fails**: `seen > 0` violated | `last()`/`top()` return a slot that holds nothing yet |
| `spsc-te-last-producer-cap2` | passes | the value read is stale but is a value that was written |
| `mptest-first-done-cap1/2` (C4) | **still fails** | the test's own end condition |
| `mptest-both-done-cap2` | passes | its only failure was the data race of C3a |

So atomic slots remove exactly the DATA RACES (they turn undefined behaviour into wrong answers) and fix C1; they do not fix C2, C3 or C4,
because those are logic errors in head, tail and count, not memory-model errors in the slots.

Conditions `std::atomic<T>` puts on `T` (and on the library): `T` must be trivially copyable (and, to be lock-free, small: on this
board and x86-64 up to 8 bytes lock-free, 16 bytes only with `cmpxchg16b`/`-latomic`; larger `T` makes `std::atomic<T>` use a hidden lock, which
defeats lock-freedom). The library stores arbitrary `T` (`std::string`, `std::pair<int, std::string>` in its own examples): those cannot be
`std::atomic<T>`. `std::vector<std::atomic<T>>` also cannot be resized or copied (the atomics are not copyable), and
`std::array<std::atomic<T>, N>` cannot be copy-assigned, which the header's copy and move members (hpp:284-471) do. The span form would need
the caller to own `std::atomic<T>` objects. A per-slot sequence number (design D) puts no condition on `T`.

## What is PROVEN safe, by thread count

PROVEN means: the arm PASSES, i.e. GenMC explored every execution of that scenario (race free, FIFO, no value lost or repeated except by the
documented overwrite, count equation, head and tail in range, count at most the capacity). Sizes are in "Arms"; read them as "up to".

| operation(s) | threads | verdict | evidence |
|---|---|---|---|
| `try_emplace` + `consume` | 1 producer, 1 consumer | **safe** (capacity 1, 2, 3; vector/span and array lines of `full()`; trivial and non-trivial `T`; 3 to 5 operations; with a prefill; with a third thread reading `size/empty/full`) | `spsc-te-consume-*` |
| `try_emplace` + `pop` | 1 + 1 | **safe** | `spsc-te-pop-*` |
| `try_emplace` + `top()` then `pop()` (same consumer thread) | 1 + 1 | **safe** | `spsc-te-top-then-pop-cap2` |
| `try_emplace` + `top_pop` | 1 + 1 | **UNSAFE as written** (C1); safe with release on hpp:1419 | `spsc-te-toppop-*`, `-fix` |
| `push` / `emplace` + `top_pop` while the buffer never fills | 1 + 1 | **safe** (3 items into capacity 3, all three kinds of push) | `spsc-push-nofull-*` |
| `push` on a full buffer (overwrite) with a consumer running | 1 + 1 | **UNSAFE** (C2, eviction) | `spsc-push-evict-*` |
| `push` on a full buffer, no other thread but a `size()` reader | 1 + observer | **safe** (the oldest goes, the newest stay, count never above capacity) | `sp-push-evict-observer-*` |
| `push` from two threads, room for all items, no consumer running | 2 producers | **safe** | `mp2-push-cap2`, `mp2-push-cap3-prefill`, `mp2x-*` |
| `push` from two threads with eviction or capacity 1 | 2 producers | **UNSAFE** (C3) | `mp2-push-cap1`, `mp2-push-evict-cap2` |
| `push` from two threads + a consumer | 2 + 1 | **UNSAFE** (C3b) | `mpsc-push-toppop-cap3` |
| `top_pop` / `pop` from two threads on two or more items already in the buffer, no producer | 2 consumers | **safe** | `mc2-prefill2-*`, `mc2x-*` |
| `top_pop` from two threads on fewer items than consumers, or a producer running | 2 consumers | **UNSAFE** (C2) | `mc2-prefill1-cap2`, `spmc-push-toppop-cap3` |
| `consume` from two threads | 2 consumers | **UNSAFE** (documented single consumer) | `mc2-consume-prefill2-cap2` |
| `try_emplace` from two threads | 2 producers | **UNSAFE** (documented single producer) | `mp2-try_emplace-cap1` |
| `last()` from the consumer or the producer thread while the other runs; `top()` from the producer thread | 1 + 1 | **UNSAFE** (reads a slot being written or destroyed, or one not yet written) | `spsc-te-last-*`, `spsc-te-top-producer-*` |
| `last()` from the pushing thread alone | 1 | safe | `sp-last-producer-cap2` |
| `size()`, `empty()`, `full()` | any, read-only | safe (one atomic load; the value is a count that may include items being claimed) | `*-observer-*` |
| capacity 0 | 1 | safe: every call answers, none stores, no slot is touched | `seq-cap0` |

Header class comment versus these results: "many producers and many consumers included" is **not** met for `push`, `emplace`,
`pop`, `top_pop`; the README's one producer and one consumer holds for `try_emplace`, `consume`, `pop` and `top()`+`pop()`, and for
`top_pop` once C1 is fixed. `push` on a full buffer is not safe even with one producer and one consumer.

## Arms and verdicts

Every arm has an EXPECTED verdict written from reading the header BEFORE it was run (`arms.py`); the runner prints a MISMATCH and exits non-zero if the actual verdict differs. Where a prediction was wrong it was corrected in
`arms.py` and the correction is recorded here: `spsc-te-top-producer-cap2-trivial-T` (predicted PASS, FAILS: `top()` reads the NEXT slot, which holds nothing, after the
consumer popped the last item), `mut-1455` at `tc2` (predicted escape, caught), the design mutants of `mp2x`/`mc2x` (predicted caught, they escape: no thread reads
what another wrote there), `design-spsc-te-last-consumer-cap2` (predicted FAIL, passes once `last()` checks the published number).
Arms that did not finish in 10 minutes were made smaller, and the arm's note says so: `spsc-te-consume-cap2-5ops` (now `cap1-5ops` and `cap2-4ops`),
`spsc-te-consume-cap2-observer` (3 operations, now 2), `mptest-both-done-cap1-3ops-norace-accounting` (now 2 operations).
On design D, GenMC's spin pass had to be handled per arm: the design's loops index a slot by a loaded ticket and the pass prunes iterations that are not spins,
so most `design-*` arms run with the pass OFF (`--disable-spin-assume`: every execution explored). Scenarios where `push` meets a thread with an item in
flight (a full buffer, a consumer between taking an item and freeing its slot, or a producer between claiming and publishing) have infinite executions on the design:
without the pass they do not finish (10 minutes, cut); they are the `design-weak-*` arms, run WITH the pass (waiting executions pruned: a fairness assumption), as
weaker evidence. A `design-mut-*` FAIL is a counter-example, so the pass does not weaken it; a PASS there is the weaker kind. Dropped for the same reason:
`design-mp2-push-evict-cap2` and a design run of `mptest-first-done`.

Groups (`run.py` names): `seq-*` one thread against a reference queue (capacity 0 to 3, every operation, wrap, overwrite, clear);
`spsc-te-*` the promised 1P1C use; `spsc-push-*`, `sp-*`, `mp2-*`, `mpsc-*`, `mc2-*`, `spmc-*` the uses the class comment claims; `mp2-try_emplace-cap1`,
`mc2-consume-*`, `spsc-te-last-*`, `spsc-te-top-producer-*` the uses it does not; `mptest-*` the library's test; `live-*`, `retry-bound-*` progress;
`fixE1-*` the C1 fix against every scenario; `atomicslots-*` the atomic-slot idea; `design-*` design D; `mut-*` mutants; `obs-*` the lifetime observations;
`fidelity` and `text-search` the static checks.

| group | arms | PASS | FAIL (counter-example) | cut off |
|---|---|---|---|---|
| static | 2 | 2 | 0 | 0 |
| sequential | 12 | 12 | 0 | 0 |
| observation | 4 | 1 | 3 | 0 |
| promised-1P1C/claimed/not-promised | 44 | 26 | 18 | 0 |
| library-test | 7 | 4 | 3 | 0 |
| progress | 8 | 6 | 2 | 0 |
| mutant | 74 | 44 | 30 | 0 |
| design | 44 | 42 | 2 | 0 |
| design-weak | 7 | 7 | 0 | 0 |
| fix-E1 | 60 | 43 | 17 | 0 |
| atomic-slots | 21 | 7 | 14 | 0 |
| design-mutant | 13 | 5 | 8 | 0 |
| total | 296 | 199 | 97 | 0 |

## Mutants

One arm per weakened or removed step: each memory order of the header's concurrent functions weakened one level (acq_rel to
release / acquire / relaxed; acquire, release to relaxed; the relaxed ones cannot be weakened), each compare-and-swap replaced by a plain store, each retry
(with the re-check it carries) removed, `fetch_add` moved before the slot write, `fetch_sub` moved before the destroy or the read, the index wrapping one slot late.
Each mutant runs against several scenarios; it is CAUGHT if any of them fails. 16 mutants are caught, 18 escape. Reading of the escapes:

* **tail CAS release (1285), head load/CAS of `read_insert` (1297, 1308), `top` head load (1378), `consume` head load (734)**: nothing reads those words with
  acquire to obtain a slot: the data is published through the count (release at 1327/1347/1368, acquire at 1449), so the order is stronger than the
  models need. `last()`'s tail load (1388) escapes because `last()` is only passed by the arm where a single thread calls it (any other thread's use fails already).
* **size load (1463)**: `size()` returns a value, no data hangs on it; the model asserts the value is at most the capacity, which a relaxed load keeps.
* **acq_rel to release on `pop` (1441)**: the acquire half is redundant: the consumer has already acquired the count in `is_empty()`.
* **every eviction-path order and structure (1762, 1767, 1780, store instead of CAS at 1766, no retry at 1766, the count lowered before the destroy at 1776)**: an
  eviction only meets another thread in the arms where the header is already wrong (C2/C3), where the baseline fails with or without the mutant; the passing eviction arms
  (`sp-push-evict-observer-*`) have no thread that could notice. The model is too small to separate these mutants from an already broken baseline.
* **no re-check of `full()` after a failed tail CAS (1281), no re-check of `is_empty()` after a failed head CAS (1299)**: they matter only when the buffer is
  full/empty at the moment a compare-and-swap fails, and the only scenarios that reach that state are the ones that already fail on C2/C3.
* **tail CAS to store, tail no retry, head CAS to store, head no retry** escape in the small scenarios (`mp2`, `mc2`) and are CAUGHT by `mp2x`/`mc2x`
  (a push or two pops after the join shows the tail or head left in the wrong place): the small scenarios were too small to see them.

| mutant (header line, change) | scenarios run | verdict |
|---|---|---|
| `1285-tail-cas-release-relaxed` | tc2:passes, mp2:passes, ev2:passes | **ESCAPES** |
| `1297-head-load-acquire-relaxed` | tc2:passes, mc2:passes, tp2:passes | **ESCAPES** |
| `1308-head-cas-release-relaxed` | tc2:passes, mc2:passes | **ESCAPES** |
| `1327-count-add-copy-release-relaxed` | pc3:caught | CAUGHT by pc3 |
| `1347-count-add-move-release-relaxed` | pm3:caught | CAUGHT by pm3 |
| `1368-count-add-emplace-release-relaxed` | tc2:caught, pe3:caught | CAUGHT by tc2, pe3 |
| `1378-top-head-acquire-relaxed` | tp2:passes | **ESCAPES** |
| `1388-last-tail-acquire-relaxed` | last:passes | **ESCAPES** |
| `1441-pop-sub-acqrel-release` | pop1:passes, pop2:passes, tc1:passes | **ESCAPES** |
| `1441-pop-sub-acqrel-acquire` | pop1:caught, pop2:caught, tc1:caught | CAUGHT by pop1, pop2, tc1 |
| `1441-pop-sub-acqrel-relaxed` | pop1:caught, pop2:caught, tc1:caught | CAUGHT by pop1, pop2, tc1 |
| `1449-empty-load-acquire-relaxed` | tc2:caught, pop2:caught | CAUGHT by tc2, pop2 |
| `1455-full-load-acquire-relaxed` | tc1:caught, tc2:caught | CAUGHT by tc1, tc2 |
| `1457-full-load-array-acquire-relaxed` | tc1a:caught | CAUGHT by tc1a |
| `1463-size-load-acquire-relaxed` | obs2:passes, tc2:passes | **ESCAPES** |
| `734-consume-head-acquire-relaxed` | tc2:passes | **ESCAPES** |
| `1762-evict-head-load-acquire-relaxed` | ev1:passes, ev2:passes | **ESCAPES** |
| `1767-evict-head-cas-release-relaxed` | ev1:passes, ev2:passes | **ESCAPES** |
| `1780-evict-sub-acqrel-release` | ev1:passes, ev2:passes | **ESCAPES** |
| `1780-evict-sub-acqrel-acquire` | ev1:passes, ev2:passes | **ESCAPES** |
| `1780-evict-sub-acqrel-relaxed` | ev1:passes, ev2:passes | **ESCAPES** |
| `1285-tail-cas-to-store` | tc2:passes, mp2:caught, mp2x:caught | CAUGHT by mp2, mp2x |
| `1307-head-cas-to-store-read_insert` | tc2:passes, mc2:caught, mc2x:caught | CAUGHT by mc2, mc2x |
| `1766-head-cas-to-store-evict` | ev1:passes, ev2:passes | **ESCAPES** |
| `1285-tail-no-retry` | tc2:passes, mp2:passes, mp2x:caught | CAUGHT by mp2x |
| `1307-head-no-retry-read_insert` | tc2:passes, mc2:passes, mc2x:caught | CAUGHT by mc2x |
| `1766-head-no-retry-evict` | ev1:passes, ev2:passes | **ESCAPES** |
| `1281-no-full-recheck-after-failed-cas` | mp2:passes, mp2x:passes, ev2:passes | **ESCAPES** |
| `1299-no-empty-recheck-after-failed-cas` | mc2:passes, mc2x:passes, tc2:passes | **ESCAPES** |
| `1321-1368-count-add-before-slot-write` | tc2:caught, pc3:caught, pm3:caught | CAUGHT by tc2, pc3, pm3 |
| `1439-1441-pop-count-sub-before-destroy` | pop2:caught, tc2:caught | CAUGHT by pop2, tc2 |
| `1411-1419-toppop-count-sub-before-read` | fix1:caught, fix2:caught | CAUGHT by fix1, fix2 |
| `1776-1780-evict-count-sub-before-destroy` | ev1:passes, ev2:passes | **ESCAPES** |
| `1501-index-wraps-one-slot-late` | seq2:caught, tc2:caught | CAUGHT by seq2, tc2 |

Mutants of the proposed design (`ring_seq.h`):

| mutant | scenarios | verdict |
|---|---|---|
| `seq-store-release-relaxed` | tc2:caught, pop1:caught, mp2x:passes, mc2x:passes, ev2:caught, mpsc:caught | CAUGHT by tc2, pop1, ev2, mpsc |
| `seq-load-acquire-relaxed` | tc2:caught, pop1:caught, mp2x:passes, mc2x:passes, ev2:caught, mpsc:caught | CAUGHT by tc2, pop1, ev2, mpsc |
| `ticket-cas-relaxed-is-enough` | mpsc:passes | escapes |

## Older observations

Two observations from the earlier review, checked on the model (arms `obs-*`, `-DLIFETIME`), not fixed:

**O1: assignment into a destroyed object (FAIL `obs-assign-into-destroyed`, `obs-evict-then-assign`).** `pop`, `top_pop` and `pop_front_unsafe` run `~T()` on the slot
(hpp:1411, 1439, 1776) and leave the object in the container. A later `push(const T&)` / `push(T&&)` does `slot = item` (hpp:1321, 1339), an `operator=` on an object whose
lifetime ended: undefined behaviour. For `int` it does nothing; for `std::string` the destroyed string has freed its buffer and `operator=` reads the dangling pointer
(use after free). `emplace` / `try_emplace` use placement `new` (hpp:1360) and are fine (`obs-emplace-after-pop` PASSES). The eviction path (`push` on a full buffer) always
destroys the slot and then assigns into it.

**O2: a second destructor (FAIL `obs-double-destroy`).** After `pop`, the container (`std::vector<T>`, `std::array<T, N>`) still holds the element, so when the buffer dies
the container's destructor runs `~T()` on every element again, including the popped one: two destructors on one object (double free for `std::string` longer than the
small-string size). Smallest fix as text: do not call `~T()` in `pop`/`top_pop`/`pop_front_unsafe` (leave the element alive until the container destroys it; assignment then
works) or store the elements in raw aligned storage and construct with placement `new` on every push, destroying exactly once on pop.

**Capacity 0**: `push`/`emplace` return before `insert()` (hpp:1317, 1333, 1354), `try_emplace` returns false because `full()` is `0 == 0`, every read returns "empty"
(`is_empty()` and `full()` are both true), `increment` is guarded against `% 0` (hpp:1501): arm `seq-cap0` PASSES and asserts that no slot is touched.

## What the models do not cover

* Capacities above 3, more than 3 threads, more than 5 operations per thread (some arms are cut at 2 to 5; GenMC's state space grows exponentially).
* Elements other than `int`: a non-trivial `T` is modelled only through "its destructor writes the slot" (`DTOR_WRITES=1`, the default) and the
  lifetime arms; copy/move constructors that throw, `operator=` that reads, `T` larger than a word are not modelled.
* The statistics (`sum`, `mean`, `variance`, `standard_deviation`, `minimum`, `maximum`, the sorted and median functions) and the atomics `m_sum`,
  `m_sum_squares` (relaxed `fetch_add`/`fetch_sub`, `atomic_float_add` CAS loops): not modelled; read as single-threaded.
* `reset()` / `clear()` under concurrency, copy, move, the iterators (they walk the plain storage with no synchronisation), the TBB parallel algorithms.
* Hardware memory models beyond RC11 (the C1 note), a real compiler's reordering of non-atomic code, and `std::atomic<T>` for non-lock-free sizes.
* Weak compare-and-swap failing spuriously more than twice per run (the `SPURIOUS` arms); real preemption (GenMC explores every interleaving, so this is covered).
* GenMC does not report a data race for the model's own harness variables, which are thread-owned until joined.

## Proposed lines for the top-level README

Suggested for the top-level README (under "Features" or a new "Thread safety"):

    - **Verified use**: one producer thread calling `try_emplace`, one consumer thread calling `consume`, `pop` or `top()` + `pop()`.
      This is model-checked (GenMC, RC11) in `verification/`; `push` on a full buffer, several producers and several consumers are NOT safe.
    - `top_pop`, `last`, and `top` from a thread that is not the consumer are not covered by that check; see `verification/README.md`.

## Correspondence table (header line to model line, checked by `run.py`, arm `fidelity`)

`ring.h` stands for the header's concurrent functions. Every `hpp:<line> "<text>"` tag in `ring.h` is checked to occur
on that header line; every `std::memory_order_*` in a concurrent function must have a `SITE` marker with the same
order; every header line of a concurrent function that touches `m_head`, `m_tail`, `m_count`, a slot, a loop or a
return must be tagged, or be listed as not modelled in `run.py` (`NOT_MODELLED`: the statistics hooks
`atomic_add`/`atomic_sub` and their `if constexpr (is_arithmetic)`). The header's sha256 is pinned in `run.py`.

| model line | header line | memory order | header text |
|---|---|---|---|
| ring.h:183 | 118 | plain | `CircularBuffer(void) : m_head(0UL), m_tail(0UL), m_count(0UL) {` |
| ring.h:184 | 118 | plain | `CircularBuffer(void) : m_head(0UL), m_tail(0UL), m_count(0UL) {` |
| ring.h:185 | 118 | plain | `CircularBuffer(void) : m_head(0UL), m_tail(0UL), m_count(0UL) {` |
| ring.h:457 | 498 | plain | `push_back(item);` |
| ring.h:458 | 526 | plain | `push_back(std::move(item));` |
| ring.h:459 | 554 | plain | `emplace_back(std::forward<Args>(args)...);` |
| ring.h:469 | 581 | plain | `bool pop(void) {` |
| ring.h:462 | 696 | plain | `if (full()) {` |
| ring.h:463 | 697 | plain | `return false;` |
| ring.h:465 | 700 | plain | `emplace_back(std::forward<Args>(args)...);` |
| ring.h:466 | 702 | plain | `return true;` |
| ring.h:524 | 730 | plain | `if (is_empty()) {` |
| ring.h:525 | 731 | plain | `return false;` |
| ring.h:527 | 734 | acquire | `std::forward<Fn>(fn)(std::as_const(this->operator[](m_head.load(std::memory_order_acquire))));` |
| ring.h:528 | 736 | plain | `return pop_front();` |
| ring.h:314 | 1274 | plain | `size_type current_tail{0UL}, next_tail{0UL};` |
| ring.h:334 | 1276 | plain | `do {` |
| ring.h:335 | 1277 | relaxed | `current_tail = m_tail.load(std::memory_order_relaxed);` |
| ring.h:336 | 1279 | plain | `next_tail = increment(current_tail);` |
| ring.h:343 | 1281 | plain | `if (full()) {` |
| ring.h:344 | 1282 | plain | `pop_front_unsafe();` |
| ring.h:347 | 1285 | release/relaxed | `} while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_r` |
| ring.h:349 | 1287 | plain | `return current_tail;` |
| ring.h:356 | 1293 | plain | `size_type current_head{0}, next_head{0};` |
| ring.h:376 | 1295 | plain | `do {` |
| ring.h:377 | 1297 | acquire | `current_head = m_head.load(std::memory_order_acquire);` |
| ring.h:384 | 1299 | plain | `if (is_empty()) {` |
| ring.h:389 | 1301 | plain | `return std::nullopt;` |
| ring.h:393 | 1305 | plain | `next_head = increment(current_head);` |
| ring.h:394 | 1307 | plain | `} while (!m_head.compare_exchange_weak(current_head, next_head,` |
| ring.h:396 | 1311 | plain | `return current_head;` |
| ring.h:403 | 1315 | plain | `void push_back(const T& item) {` |
| ring.h:404 | 1317 | plain | `if (get_capacity() == 0UL) {` |
| ring.h:405 | 1318 | plain | `return; // a buffer with no slots takes nothing` |
| ring.h:415 | 1321 | plain | `this->operator[](insert()) = item;` |
| ring.h:416 | 1327 | release | `m_count.fetch_add(1, std::memory_order_release);` |
| ring.h:420 | 1331 | plain | `void push_back(T&& item) {` |
| ring.h:421 | 1333 | plain | `if (get_capacity() == 0UL) {` |
| ring.h:422 | 1334 | plain | `return; // a buffer with no slots takes nothing` |
| ring.h:424 | 1337 | plain | `const size_type current_tail = insert();` |
| ring.h:432 | 1339 | plain | `this->operator[](current_tail) = std::move(item);` |
| ring.h:433 | 1347 | release | `m_count.fetch_add(1, std::memory_order_release);` |
| ring.h:437 | 1352 | plain | `void emplace_back(Args&&... args) {` |
| ring.h:438 | 1354 | plain | `if (get_capacity() == 0UL) {` |
| ring.h:439 | 1355 | plain | `return; // a buffer with no slots takes nothing` |
| ring.h:441 | 1358 | plain | `const size_type current_tail = insert();` |
| ring.h:449 | 1360 | plain | `new (&this->operator[](current_tail)) T(std::forward<Args>(args)...);  // Construct in-place` |
| ring.h:450 | 1368 | release | `m_count.fetch_add(1, std::memory_order_release);` |
| ring.h:503 | 1374 | plain | `if (is_empty()) {` |
| ring.h:504 | 1375 | plain | `return std::nullopt;` |
| ring.h:506 | 1378 | acquire | `return this->operator[](m_head.load(std::memory_order_acquire));` |
| ring.h:511 | 1384 | plain | `if (is_empty()) {` |
| ring.h:512 | 1385 | plain | `return std::nullopt;` |
| ring.h:516 | 1388 | acquire | `size_type _tail_position = m_tail.load(std::memory_order_acquire);` |
| ring.h:514 | 1389 | plain | `if constexpr((N > 0 && N != std::dynamic_extent)) {` |
| ring.h:517 | 1390 | plain | `_tail_position = (_tail_position == 0) ? (N - 1) : (_tail_position - 1);` |
| ring.h:515 | 1393 | plain | `if constexpr((N == 0 \|\| N == std::dynamic_extent)) {` |
| ring.h:515 | 1394 | plain | `_tail_position = (_tail_position == 0) ? (m_max_size - 1) : (_tail_position - 1);` |
| ring.h:518 | 1397 | plain | `return this->operator[](_tail_position);` |
| ring.h:486 | 1403 | plain | `const std::optional<size_type> current_head = read_insert();` |
| ring.h:486 | 1405 | plain | `if (!current_head) {` |
| ring.h:487 | 1406 | plain | `return std::nullopt;` |
| ring.h:494 | 1409 | plain | `T value = this->operator[](current_head.value());` |
| ring.h:495 | 1411 | plain | `this->operator[](current_head.value()).~T();` |
| ring.h:496 | 1419 | relaxed | `m_count.fetch_sub(1, std::memory_order_relaxed);` |
| ring.h:498 | 1421 | plain | `return value;` |
| ring.h:471 | 1427 | plain | `const std::optional<size_type> current_head = read_insert();` |
| ring.h:471 | 1429 | plain | `if (!current_head) {` |
| ring.h:472 | 1430 | plain | `return false;` |
| ring.h:478 | 1439 | plain | `this->operator[](current_head.value()).~T();` |
| ring.h:479 | 1441 | acq_rel | `m_count.fetch_sub(1, std::memory_order_acq_rel);` |
| ring.h:481 | 1443 | plain | `return true;` |
| ring.h:254 | 1449 | acquire | `return m_count.load(std::memory_order_acquire) == 0;` |
| ring.h:260 | 1453 | plain | `bool full(void) const {` |
| ring.h:261 | 1454 | plain | `if constexpr ((N == 0 \|\| N == std::dynamic_extent)) {` |
| ring.h:265 | 1455 | acquire | `return m_count.load(std::memory_order_acquire) == m_max_size;` |
| ring.h:263 | 1457 | acquire | `return m_count.load(std::memory_order_acquire) == N;` |
| ring.h:270 | 1463 | acquire | `return m_count.load(std::memory_order_acquire);` |
| ring.h:273 | 1467 | plain | `size_type get_capacity(void) const {` |
| ring.h:274 | 1469 | plain | `if constexpr ((N > 0 && N != std::dynamic_extent)) {` |
| ring.h:275 | 1470 | plain | `return N;` |
| ring.h:275 | 1472 | plain | `return m_max_size;` |
| ring.h:532 | 1489 | plain | `while (!is_empty()) {` |
| ring.h:533 | 1491 | plain | `if(!pop_front()){` |
| ring.h:534 | 1492 | plain | `break;` |
| ring.h:244 | 1499 | plain | `size_type increment(const size_type& value) const {` |
| ring.h:245 | 1500 | plain | `if constexpr((N == 0 \|\| N == std::dynamic_extent)) {` |
| ring.h:249 | 1501 | plain | `return (value + 1) % (m_max_size == 0 ? 1 : m_max_size);` |
| ring.h:249 | 1503 | plain | `return (value + 1) % N;` |
| ring.h:146 | 1752 | plain | `std::atomic<size_type> m_head, m_tail, m_count;` |
| ring.h:147 | 1752 | plain | `std::atomic<size_type> m_head, m_tail, m_count;` |
| ring.h:148 | 1752 | plain | `std::atomic<size_type> m_head, m_tail, m_count;` |
| ring.h:282 | 1758 | plain | `size_type current_head{0}, next_head{0};` |
| ring.h:293 | 1760 | plain | `do {` |
| ring.h:294 | 1762 | acquire | `current_head = m_head.load(std::memory_order_acquire);` |
| ring.h:295 | 1764 | plain | `next_head = increment(current_head);` |
| ring.h:296 | 1766 | plain | `} while (!m_head.compare_exchange_weak(current_head, next_head,` |
| ring.h:302 | 1776 | plain | `this->operator[](current_head).~T();` |
| ring.h:303 | 1780 | acq_rel | `m_count.fetch_sub(1, std::memory_order_acq_rel); // Using acq_rel for decrement to ensure total order with inc` |

| header line | memory order | model macro | model line |
|---|---|---|---|
| 734 | acquire | O_HEAD_LOAD_CONS | ring.h:122 |
| 1277 | relaxed | O_TAIL_LOAD | ring.h:54 |
| 1285 | release | O_TAIL_CAS_OK | ring.h:58 |
| 1285 | relaxed | O_TAIL_CAS_FAIL | ring.h:62 |
| 1297 | acquire | O_HEAD_LOAD_RI | ring.h:66 |
| 1308 | release | O_HEAD_CAS_RI_OK | ring.h:70 |
| 1309 | relaxed | O_HEAD_CAS_RI_FAIL | ring.h:74 |
| 1327 | release | O_COUNT_ADD_COPY | ring.h:78 |
| 1347 | release | O_COUNT_ADD_MOVE | ring.h:82 |
| 1368 | release | O_COUNT_ADD_EMPL | ring.h:86 |
| 1378 | acquire | O_HEAD_LOAD_TOP | ring.h:90 |
| 1388 | acquire | O_TAIL_LOAD_LAST | ring.h:94 |
| 1419 | relaxed | O_COUNT_SUB_TOPPOP | ring.h:98 |
| 1441 | acq_rel | O_COUNT_SUB_POP | ring.h:102 |
| 1449 | acquire | O_COUNT_LOAD_EMPTY | ring.h:106 |
| 1455 | acquire | O_COUNT_LOAD_FULL | ring.h:110 |
| 1457 | acquire | O_COUNT_LOAD_FULLN | ring.h:114 |
| 1463 | acquire | O_COUNT_LOAD_SIZE | ring.h:118 |
| 1762 | acquire | O_HEAD_LOAD_EV | ring.h:126 |
| 1767 | release | O_HEAD_CAS_EV_OK | ring.h:130 |
| 1768 | relaxed | O_HEAD_CAS_EV_FAIL | ring.h:134 |
| 1780 | acq_rel | O_COUNT_SUB_EV | ring.h:138 |


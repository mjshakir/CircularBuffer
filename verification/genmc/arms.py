"""The table of arms of run.py: name, model file, -D flags, expected verdict, group.

The expected verdicts were written from a reading of include/CircularBuffer.hpp before the runs (README, "what
was predicted"). PASS: GenMC explores every execution of the scenario and finds no race, no assertion failure,
no liveness violation. FAIL: it finds one (the arm documents a defect or a use the header does not support).
For a mutant arm, FAIL means the mutant is caught by that scenario and PASS means it escapes there.
"""

LIVE = ["--check-liveness"]


def define(arm_raw):
    registry = []

    def arm(name, src, defs, expect, group, note="", flags=None):
        registry.append((name, src, defs, expect, group, note, flags))
        arm_raw(name, src, defs, expect, group, note, flags)

    def seq(name, defs, expect, group="sequential", note=""):
        arm(name, "sequential.c", defs, expect, group, note)

    def sc(name, defs, expect, group, note="", flags=None):
        arm(name, "scenario.c", defs, expect, group, note, flags)

    # ------------------------------------------------------------------------------------------------------
    # One thread: the buffer against a reference queue; fill, refuse, overwrite, wrap, drain, clear
    # ------------------------------------------------------------------------------------------------------
    scripts = {
        "fill-drain": "tttttqqqqq",       # try_emplace until it refuses, then drain until empty answers
        "overwrite": "pmepmepmeqqoccx",   # push, move, emplace past the capacity; drain; empty answers; clear
        "mixed": "tpqteoqmcpxtoqpcee",    # every operation, wrapping the indices
    }
    for cap in (1, 2, 3):
        for key, script in scripts.items():
            seq("seq-cap%d-%s" % (cap, key), ["CAP=%d" % cap, "SCRIPT=" + script], "PASS")
    seq("seq-cap0", ["CAP=0", "SCRIPT=tpmeoqcxtpmeoqcx"], "PASS", note="a buffer without slots stores nothing and touches no slot")
    seq("seq-cap2-array", ["CAP=2", "STORAGE_ARRAY=1", "SCRIPT=" + scripts["mixed"]], "PASS")
    seq("seq-cap1-trivial-T", ["CAP=1", "DTOR_WRITES=0", "SCRIPT=" + scripts["overwrite"]], "PASS")
    # the older observations: the lifetime of a non-trivial T
    seq("obs-assign-into-destroyed", ["CAP=1", "OBS=1", "LIFETIME=1"], "FAIL", "observation",
        "pop() runs ~T() on the slot, push(T&&) then assigns into it (operator= on a destroyed object)")
    seq("obs-evict-then-assign", ["CAP=2", "OBS=4", "LIFETIME=1"], "FAIL", "observation",
        "push on a full buffer destroys the oldest, then assigns the new item into a destroyed slot")
    seq("obs-double-destroy", ["CAP=1", "OBS=2", "LIFETIME=2"], "FAIL", "observation",
        "pop() destroyed the slot; the vector/array destroys every element again when the buffer dies")
    seq("obs-emplace-after-pop", ["CAP=1", "OBS=3", "LIFETIME=3"], "PASS", "observation",
        "placement new on a destroyed slot is fine; the later destructor runs once")

    # ------------------------------------------------------------------------------------------------------
    # What the header promises: try_emplace + consume, one producer, one consumer
    # ------------------------------------------------------------------------------------------------------
    g = "promised-1P1C"
    sc("spsc-te-consume-cap1", ["CAP=1"], "PASS", g)
    sc("spsc-te-consume-cap2", ["CAP=2"], "PASS", g)
    sc("spsc-te-consume-cap3", ["CAP=3"], "PASS", g)
    sc("spsc-te-consume-cap1-5ops", ["CAP=1", "PROD_OPS=5", "CONS_OPS=5"], "PASS", g, "one slot reused five times")
    sc("spsc-te-consume-cap2-4ops", ["CAP=2", "PROD_OPS=4", "CONS_OPS=4"], "PASS", g,
       "the indices wrap; reduced from 5 operations, which GenMC did not finish in 10 minutes")
    sc("spsc-te-consume-cap2-array", ["CAP=2", "STORAGE_ARRAY=1"], "PASS", g, "the std::array line of full()")
    sc("spsc-te-consume-cap2-trivial-T", ["CAP=2", "DTOR_WRITES=0"], "PASS", g)
    sc("spsc-te-consume-cap2-prefill", ["CAP=2", "PREFILL=1"], "PASS", g)
    sc("spsc-te-pop-cap1", ["CAP=1", "CONS_KIND=2"], "PASS", g, "pop() answers true/false and reads nothing")
    sc("spsc-te-pop-cap2", ["CAP=2", "CONS_KIND=2"], "PASS", g)
    sc("spsc-te-top-then-pop-cap2", ["CAP=2", "CONS_KIND=3"], "PASS", g, "the consumer calls top() then pop()")
    sc("spsc-te-consume-cap2-observer", ["CAP=2", "OBSERVER=1", "PROD_OPS=2", "CONS_OPS=2"], "PASS", g,
       "a third thread reads size(), empty(), full(); reduced from 3 operations, which GenMC did not finish in 10 minutes")

    # ------------------------------------------------------------------------------------------------------
    # What the class comment claims for every other function ("many producers and many consumers included")
    # ------------------------------------------------------------------------------------------------------
    g = "claimed-others"
    # top_pop: the count is lowered with a RELAXED fetch_sub (hpp:1419), so the room is not ordered after the slot read
    sc("spsc-te-toppop-cap1", ["CAP=1", "CONS_KIND=0"], "FAIL", g, "DEFECT D1: race on the slot read by top_pop")
    sc("spsc-te-toppop-cap2", ["CAP=2", "CONS_KIND=0"], "FAIL", g, "DEFECT D1")
    sc("spsc-te-toppop-cap1-trivial-T", ["CAP=1", "CONS_KIND=0", "DTOR_WRITES=0"], "FAIL", g, "D1 does not need a destructor")
    sc("spsc-te-toppop-cap1-fix", ["CAP=1", "CONS_KIND=0", "O_COUNT_SUB_TOPPOP=memory_order_release"], "PASS", g,
       "the smallest fix of D1: release on hpp:1419")
    sc("spsc-te-toppop-cap2-fix", ["CAP=2", "CONS_KIND=0", "O_COUNT_SUB_TOPPOP=memory_order_release"], "PASS", g)
    # push never reaches a full buffer: no eviction, the producer and the consumer meet only through the count
    sc("spsc-push-nofull-copy-cap3", ["CAP=3", "PROD_KIND=1", "CONS_KIND=0"], "PASS", g, "push(const T&), room for every item")
    sc("spsc-push-nofull-move-cap3", ["CAP=3", "PROD_KIND=2", "CONS_KIND=0"], "PASS", g, "push(T&&)")
    sc("spsc-push-nofull-emplace-cap3", ["CAP=3", "PROD_KIND=3", "CONS_KIND=0"], "PASS", g, "emplace")
    # no consumer thread: the producer evicts, an observer reads the count
    sc("sp-push-evict-observer-cap1", ["CAP=1", "PROD_KIND=1", "PROD_OPS=4", "NCONS=0", "OBSERVER=1"], "PASS", g,
       "overwrite-on-full by one thread: the oldest goes, the newest stay, the count never passes the capacity")
    sc("sp-push-evict-observer-cap2", ["CAP=2", "PROD_KIND=1", "PROD_OPS=4", "NCONS=0", "OBSERVER=1"], "PASS", g)
    sc("sp-last-producer-cap2", ["CAP=2", "PROD_KIND=1", "PROD_OPS=3", "NCONS=0", "PROD_LAST=1"], "PASS", g, "last() from the pushing thread, no other thread")
    # push on a full buffer while a consumer pops: the eviction reads a count the consumer is about to lower
    sc("spsc-push-evict-toppop-cap1", ["CAP=1", "PROD_KIND=1", "CONS_KIND=0"], "FAIL", g, "DEFECT D2: eviction against a concurrent pop")
    sc("spsc-push-evict-toppop-cap2", ["CAP=2", "PROD_KIND=1", "CONS_KIND=0"], "FAIL", g, "DEFECT D2")
    sc("spsc-push-evict-pop-cap2", ["CAP=2", "PROD_KIND=1", "CONS_KIND=2"], "FAIL", g, "DEFECT D2 with pop()")
    sc("spsc-push-evict-toppop-cap2-fix", ["CAP=2", "PROD_KIND=1", "CONS_KIND=0", "O_COUNT_SUB_TOPPOP=memory_order_acq_rel"], "FAIL", g,
       "D2 is not an ordering defect: acq_rel on hpp:1419 does not cure it")
    # many producers
    sc("mp2-push-cap2", ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0"], "PASS", g, "two producers, room for both, no consumer thread")
    sc("mp2-push-cap3-prefill", ["CAP=3", "NPROD=2", "PROD_OPS=1", "PROD_KIND=3", "PREFILL=1", "NCONS=0"], "PASS", g)
    sc("mp2-push-evict-cap2", ["CAP=2", "NPROD=2", "PROD_OPS=2", "PROD_KIND=1", "NCONS=0"], "FAIL", g, "DEFECT D3: two producers evict on one full look")
    sc("mp2-push-cap1", ["CAP=1", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0"], "FAIL", g, "DEFECT D3: capacity 1, both claim slot 0")
    sc("mpsc-push-toppop-cap3", ["CAP=3", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "CONS_KIND=0", "CONS_OPS=2"], "FAIL", g,
       "DEFECT D4: the count is raised by the writer of the later slot, the consumer reads the earlier one unwritten")
    # many consumers
    sc("mc2-prefill2-cap2", ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0"], "PASS", g, "two consumers, two items")
    sc("mp2x-push-cap2", ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0", "AFTER_PUSH=1"], "PASS", g,
       "as mp2-push-cap2, then main pushes once more: the tail was left in the right place")
    sc("mc2x-prefill2-cap2", ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0", "AFTER_PUSH=1"], "PASS", g,
       "as mc2-prefill2-cap2, then main pushes once more: the head was left in the right place")
    sc("mc2-prefill2-pop-cap2", ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=2"], "PASS", g)
    sc("mc2-prefill1-cap2", ["CAP=2", "PREFILL=1", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0"], "FAIL", g,
       "DEFECT D5: the head moves before the count, the second consumer claims a slot that holds nothing")
    sc("spmc-push-toppop-cap3", ["CAP=3", "PROD_KIND=1", "PROD_OPS=2", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0"], "FAIL", g, "DEFECT D5 with a live producer")

    # ------------------------------------------------------------------------------------------------------
    # The nearest uses the header says are NOT supported (try_emplace / consume: one producer, one consumer)
    # and calls that read where another thread writes
    # ------------------------------------------------------------------------------------------------------
    g = "not-promised"
    sc("mp2-try_emplace-cap1", ["CAP=1", "NPROD=2", "PROD_OPS=1", "NCONS=0"], "FAIL", g, "two producers on try_emplace: both pass full()")
    sc("mc2-consume-prefill2-cap2", ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=1"], "FAIL", g,
       "two consumers on consume: both read the same head slot")
    sc("spsc-te-last-consumer-cap2", ["CAP=2", "CONS_LAST=1"], "FAIL", g, "last() from the consumer reads a slot the producer is still writing")
    sc("spsc-te-last-producer-cap2", ["CAP=2", "PROD_LAST=1"], "FAIL", g, "last() from the producer reads the slot the consumer pops and destroys")
    sc("spsc-te-top-producer-cap2", ["CAP=2", "PROD_TOP=1"], "FAIL", g, "top() from the producer reads the slot the consumer destroys (non-trivial T)")
    sc("spsc-te-top-producer-cap2-trivial-T", ["CAP=2", "PROD_TOP=1", "DTOR_WRITES=0"], "FAIL", g,
       "even with a trivial ~T(): top() sees count > 0, the consumer pops the item, top() reads the next slot, which holds nothing")

    # ------------------------------------------------------------------------------------------------------
    # The library's own test MultipleProducersSingleConsumer, modelled (mp_test.c)
    # ------------------------------------------------------------------------------------------------------
    g = "library-test"
    arm("mptest-first-done-cap1", "mp_test.c", ["CAP=1"], "FAIL", g,
        "the test as written: the consumer starts when the FIRST producer is done and stops at the first empty() it reads")
    arm("mptest-first-done-cap2", "mp_test.c", ["CAP=2"], "FAIL", g, "same, capacity 2")
    arm("mptest-both-done-cap2", "mp_test.c", ["CAP=2", "WAIT_BOTH=1"], "FAIL", g,
        "the consumer starts only when both producers are done: the two producers still race on a slot (D3)")
    arm("mptest-both-done-cap1-norace", "mp_test.c", ["CAP=1", "WAIT_BOTH=1"], "PASS", g,
        "with data-race detection OFF: whatever the producers do to the slots, the consumer started after both ends with empty()",
        ["--disable-race-detection"])
    arm("mptest-both-done-cap2-norace", "mp_test.c", ["CAP=2", "WAIT_BOTH=1"], "PASS", g, "same, capacity 2", ["--disable-race-detection"])
    arm("mptest-both-done-cap2-norace-accounting", "mp_test.c", ["CAP=2", "WAIT_BOTH=1", "WHAT=1"], "PASS", g,
        "and the count equals pushes - pops - evictions", ["--disable-race-detection"])

    # ------------------------------------------------------------------------------------------------------
    # Progress: no execution in which a thread spins for ever; every retry has a cause and is bounded
    # ------------------------------------------------------------------------------------------------------
    g = "progress"
    sc("live-spsc-te-consume-cap2", ["CAP=2"], "PASS", g, "no spinning thread", LIVE)
    sc("live-spsc-te-pop-cap1", ["CAP=1", "CONS_KIND=2"], "PASS", g, "", LIVE)
    sc("live-mp2-push-cap2", ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0", "RETRY_BOUND=1"], "PASS", g,
       "two producers on one tail: a retry has a cause (the other claim) and happens at most once", LIVE)
    sc("live-mc2-prefill2-cap2", ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0", "RETRY_BOUND=1"], "PASS", g,
       "two consumers on one head", LIVE)
    sc("live-spurious-spsc-te-consume-cap2", ["CAP=2", "SPURIOUS=2", "RETRY_BOUND=2"], "PASS", g,
       "two spurious compare-and-swap failures: each costs one retry, no more", LIVE)
    sc("live-spurious-mp2-push-cap2", ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0", "SPURIOUS=1", "RETRY_BOUND=2"], "PASS", g, "", LIVE)
    sc("retry-bound-0-mp2-push-cap2", ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0", "RETRY_BOUND=0"], "FAIL", g,
       "control: the bound check does fire when a retry happens (bound 0 is wrong)")
    sc("live-mutant-wait-empty", ["CAP=2", "NPROD=0", "CONS_KIND=0", "CONS_OPS=1", "M_WAIT_EMPTY"], "FAIL", g,
       "control: a pop that waits for an item on an empty buffer IS caught as a liveness violation", LIVE)

    # ------------------------------------------------------------------------------------------------------
    # Mutants: (tag, -D list, {killer: expected}); FAIL = caught by that scenario, PASS = escapes there
    # ------------------------------------------------------------------------------------------------------
    killers = {
        "tc1":  ["CAP=1"],
        "tc2":  ["CAP=2"],
        "tc1a": ["CAP=1", "STORAGE_ARRAY=1"],
        "pop1": ["CAP=1", "CONS_KIND=2"],
        "pop2": ["CAP=2", "CONS_KIND=2"],
        "tp2":  ["CAP=2", "CONS_KIND=3"],
        "obs2": ["CAP=2", "OBSERVER=1", "PROD_OPS=2", "CONS_OPS=2"],
        "pc3":  ["CAP=3", "PROD_KIND=1", "CONS_KIND=0"],
        "pm3":  ["CAP=3", "PROD_KIND=2", "CONS_KIND=0"],
        "pe3":  ["CAP=3", "PROD_KIND=3", "CONS_KIND=0"],
        "fix1": ["CAP=1", "CONS_KIND=0", "O_COUNT_SUB_TOPPOP=memory_order_release"],
        "fix2": ["CAP=2", "CONS_KIND=0", "O_COUNT_SUB_TOPPOP=memory_order_release"],
        "mp2":  ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0"],
        "mc2":  ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0"],
        "mp2x": ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0", "AFTER_PUSH=1"],
        "mc2x": ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0", "AFTER_PUSH=1"],
        "ev1":  ["CAP=1", "PROD_KIND=1", "PROD_OPS=4", "NCONS=0", "OBSERVER=1"],
        "ev2":  ["CAP=2", "PROD_KIND=1", "PROD_OPS=4", "NCONS=0", "OBSERVER=1"],
        "last": ["CAP=2", "PROD_KIND=1", "PROD_OPS=3", "NCONS=0", "PROD_LAST=1"],
    }
    seq_mixed = lambda cap: ["CAP=%d" % cap, "SCRIPT=" + scripts["mixed"]]
    killers_seq = {"seq1": seq_mixed(1), "seq2": seq_mixed(2)}

    def order(tag, macro, value, ks, caught):
        mutant(tag, ["%s=memory_order_%s" % (macro, value)], ks, caught)

    def mutant(tag, defs, ks, caught):
        for k in ks:
            if k in killers_seq:
                arm("mut-%s@%s" % (tag, k), "sequential.c", killers_seq[k] + defs, "FAIL" if k in caught else "PASS", "mutant")
            else:
                arm("mut-%s@%s" % (tag, k), "scenario.c", killers[k] + defs, "FAIL" if k in caught else "PASS", "mutant")

    # memory orders weakened one level (hpp line in the tag)
    order("1285-tail-cas-release-relaxed", "O_TAIL_CAS_OK", "relaxed", ["tc2", "mp2", "ev2"], [])
    order("1297-head-load-acquire-relaxed", "O_HEAD_LOAD_RI", "relaxed", ["tc2", "mc2", "tp2"], [])
    order("1308-head-cas-release-relaxed", "O_HEAD_CAS_RI_OK", "relaxed", ["tc2", "mc2"], [])
    order("1327-count-add-copy-release-relaxed", "O_COUNT_ADD_COPY", "relaxed", ["pc3"], ["pc3"])
    order("1347-count-add-move-release-relaxed", "O_COUNT_ADD_MOVE", "relaxed", ["pm3"], ["pm3"])
    order("1368-count-add-emplace-release-relaxed", "O_COUNT_ADD_EMPL", "relaxed", ["tc2", "pe3"], ["tc2", "pe3"])
    order("1378-top-head-acquire-relaxed", "O_HEAD_LOAD_TOP", "relaxed", ["tp2"], [])
    order("1388-last-tail-acquire-relaxed", "O_TAIL_LOAD_LAST", "relaxed", ["last"], [])
    order("1441-pop-sub-acqrel-release", "O_COUNT_SUB_POP", "release", ["pop1", "pop2", "tc1"], [])
    order("1441-pop-sub-acqrel-acquire", "O_COUNT_SUB_POP", "acquire", ["pop1", "pop2", "tc1"], ["pop1", "pop2", "tc1"])
    order("1441-pop-sub-acqrel-relaxed", "O_COUNT_SUB_POP", "relaxed", ["pop1", "pop2", "tc1"], ["pop1", "pop2", "tc1"])
    order("1449-empty-load-acquire-relaxed", "O_COUNT_LOAD_EMPTY", "relaxed", ["tc2", "pop2"], ["tc2", "pop2"])
    order("1455-full-load-acquire-relaxed", "O_COUNT_LOAD_FULL", "relaxed", ["tc1", "tc2"], ["tc1", "tc2"])
    order("1457-full-load-array-acquire-relaxed", "O_COUNT_LOAD_FULLN", "relaxed", ["tc1a"], ["tc1a"])
    order("1463-size-load-acquire-relaxed", "O_COUNT_LOAD_SIZE", "relaxed", ["obs2", "tc2"], [])
    order("734-consume-head-acquire-relaxed", "O_HEAD_LOAD_CONS", "relaxed", ["tc2"], [])
    order("1762-evict-head-load-acquire-relaxed", "O_HEAD_LOAD_EV", "relaxed", ["ev1", "ev2"], [])
    order("1767-evict-head-cas-release-relaxed", "O_HEAD_CAS_EV_OK", "relaxed", ["ev1", "ev2"], [])
    order("1780-evict-sub-acqrel-release", "O_COUNT_SUB_EV", "release", ["ev1", "ev2"], [])
    order("1780-evict-sub-acqrel-acquire", "O_COUNT_SUB_EV", "acquire", ["ev1", "ev2"], [])
    order("1780-evict-sub-acqrel-relaxed", "O_COUNT_SUB_EV", "relaxed", ["ev1", "ev2"], [])
    # compare-and-swap replaced by a plain store
    mutant("1285-tail-cas-to-store", ["M_TAIL_STORE"], ["tc2", "mp2", "mp2x"], ["mp2", "mp2x"])
    mutant("1307-head-cas-to-store-read_insert", ["M_HEAD_STORE_RI"], ["tc2", "mc2", "mc2x"], ["mc2", "mc2x"])
    mutant("1766-head-cas-to-store-evict", ["M_HEAD_STORE_EV"], ["ev1", "ev2"], [])
    # the retry (and the re-check it carries) removed
    mutant("1285-tail-no-retry", ["M_NO_RETRY_TAIL"], ["tc2", "mp2", "mp2x"], ["mp2x"])
    mutant("1307-head-no-retry-read_insert", ["M_NO_RETRY_RI"], ["tc2", "mc2", "mc2x"], ["mc2x"])
    mutant("1766-head-no-retry-evict", ["M_NO_RETRY_EV"], ["ev1", "ev2"], [])
    mutant("1281-no-full-recheck-after-failed-cas", ["M_NO_FULL_RECHECK"], ["mp2", "mp2x", "ev2"], [])
    mutant("1299-no-empty-recheck-after-failed-cas", ["M_NO_EMPTY_RECHECK"], ["mc2", "mc2x", "tc2"], [])
    # the count moved before / after the slot access
    mutant("1321-1368-count-add-before-slot-write", ["M_COUNT_BEFORE_WRITE"], ["tc2", "pc3", "pm3"], ["tc2", "pc3", "pm3"])
    mutant("1439-1441-pop-count-sub-before-destroy", ["M_SUB_BEFORE_DESTROY_POP"], ["pop2", "tc2"], ["pop2", "tc2"])
    mutant("1411-1419-toppop-count-sub-before-read", ["M_SUB_BEFORE_READ_TOPPOP"], ["fix1", "fix2"], ["fix1", "fix2"])
    mutant("1776-1780-evict-count-sub-before-destroy", ["M_SUB_BEFORE_DESTROY_EV"], ["ev1", "ev2"], [])
    mutant("1501-index-wraps-one-slot-late", ["M_BAD_WRAP"], ["seq2", "tc2"], ["seq2", "tc2"])

    # ------------------------------------------------------------------------------------------------------
    # The proposed design (ring_seq.h): the same scenarios, the same checks, -DRING_SEQ
    # ------------------------------------------------------------------------------------------------------
    def nospin(flags):
        # GenMC's spin-assume pass prunes a loop iteration it takes for a pure spin; the design's loops index a
        # slot by a loaded ticket, so the pass would prune live executions. Without it every execution is explored
        # (and a loop that could run for ever would keep the run from ending).
        return list(flags) + ["--disable-spin-assume"]

    skip = ("fix", "trivial", "norace", "retry", "wait-empty", "spurious", "array", "obs-", "5ops", "4ops", "mptest-first-done")
    fails_by_design = ("spsc-te-last-producer", "top-producer")  # reading from another thread; the test's own early stop
    for name, src, defs, expect, group, note, flags in registry:
        if group in ("mutant", "observation") or any(k in name for k in skip):
            continue
        if src == "sequential.c" and not name.startswith("seq-"):
            continue
        want = "FAIL" if any(k in name for k in fails_by_design) else "PASS"
        note_d = "the same scenario on the sequence-number design"
        if name in ("spsc-push-evict-toppop-cap1", "spsc-push-evict-toppop-cap2", "spsc-push-evict-pop-cap2", "mp2-push-cap1",
                    "mptest-both-done-cap2"):
            # On the design, push on a full buffer WAITS for a thread that has an item in flight (a consumer that has taken an item and not yet
            # freed its slot, or a producer that has claimed a ticket and not yet published), so these scenarios have infinite executions: without GenMC's spin pass the run does not end (10 minutes cut). With the
            # pass, executions that wait are pruned (a fairness assumption: the consumer finishes), and the pass can also prune
            # executions that do not wait: the verdict is weaker evidence than the other design arms.
            arm_raw("design-weak-" + name, src, defs + ["RING_SEQ"], "PASS", "design-weak",
                    "weaker evidence: GenMC's spin pass is on (pruned executions are counted as +Nb); push on a full buffer waits for a thread with an item in flight", flags)
            continue
        if name == "mp2-push-evict-cap2":
            continue  # same shape as mp2-push-cap1 on the design; too large without the spin pass
        arm_raw("design-" + name, src, defs + ["RING_SEQ"], want, "design", note_d,
                nospin([f for f in (flags or []) if f != "--check-liveness"]))

    # ------------------------------------------------------------------------------------------------------
    # Fix of cause E1 (release on hpp:1419) against EVERY non-mutant scenario of the header: only the
    # top_pop arms change; no other arm changes its verdict
    # ------------------------------------------------------------------------------------------------------
    for name, src, defs, expect, group, note, flags in list(registry):
        if group in ("mutant", "observation", "design") or any(k in name for k in ("fix", "norace", "retry", "wait-empty", "5ops", "4ops", "obs-")):
            continue
        if src == "sequential.c" and not name.startswith("seq-"):
            continue
        want = "PASS" if name.startswith("spsc-te-toppop") else expect
        arm_raw("fixE1-" + name, src, defs + ["O_COUNT_SUB_TOPPOP=memory_order_release"], want, "fix-E1",
                "the same scenario with release on hpp:1419", flags)

    # ------------------------------------------------------------------------------------------------------
    # The idea: the slots as std::atomic<T> (SLOTS_ATOMIC, nothing else changed; T is then trivially
    # copyable, so ~T() writes nothing: DTOR_WRITES=0). Which failures survive?
    # ------------------------------------------------------------------------------------------------------
    atomic_expect = {
        "spsc-te-toppop-cap1": "PASS", "spsc-te-toppop-cap2": "PASS",
        "spsc-push-evict-toppop-cap1": "FAIL", "spsc-push-evict-toppop-cap2": "FAIL", "spsc-push-evict-pop-cap2": "FAIL",
        "mp2-push-evict-cap2": "FAIL", "mp2-push-cap1": "FAIL", "mpsc-push-toppop-cap3": "FAIL",
        "mc2-prefill1-cap2": "FAIL", "spmc-push-toppop-cap3": "FAIL",
        "mp2-try_emplace-cap1": "FAIL", "mc2-consume-prefill2-cap2": "FAIL",
        "spsc-te-last-consumer-cap2": "FAIL", "spsc-te-last-producer-cap2": "PASS", "spsc-te-top-producer-cap2": "FAIL",
        "mptest-first-done-cap1": "FAIL", "mptest-first-done-cap2": "FAIL", "mptest-both-done-cap2": "PASS",
        "spsc-te-consume-cap2": "PASS", "mp2-push-cap2": "PASS", "mc2-prefill2-cap2": "PASS",
    }
    for name, src, defs, expect, group, note, flags in list(registry):
        if name in atomic_expect:
            arm_raw("atomicslots-" + name, src, defs + ["SLOTS_ATOMIC=1", "DTOR_WRITES=0"], atomic_expect[name], "atomic-slots",
                    "the slots are std::atomic<T> (seq_cst); nothing else changed", flags)

    # ------------------------------------------------------------------------------------------------------
    # The design on the heavier scenarios, and mutants of the design
    # ------------------------------------------------------------------------------------------------------
    arm_raw("design-weak-mptest-both-done-cap1-3ops", "mp_test.c", ["CAP=1", "WAIT_BOTH=1", "PROD_OPS=3", "RING_SEQ"], "PASS", "design-weak",
            "two producers overwriting a one-slot ring, then the consumer: ends empty, race free (spin pass on: weaker evidence)", [])
    arm_raw("design-weak-mptest-both-done-cap2-accounting", "mp_test.c", ["CAP=2", "WAIT_BOTH=1", "WHAT=1", "RING_SEQ"], "PASS", "design-weak",
            "and the count equals pushes - pops - evictions (spin pass on: weaker evidence)", [])
    dk = {
        "tc2":  ["CAP=2"],
        "pop1": ["CAP=1", "CONS_KIND=2"],
        "mp2x": ["CAP=2", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "NCONS=0", "AFTER_PUSH=1"],
        "mc2x": ["CAP=2", "PREFILL=2", "NPROD=0", "NCONS=2", "CONS_OPS=1", "CONS_KIND=0", "AFTER_PUSH=1"],
        "ev2":  ["CAP=2", "PROD_KIND=1", "CONS_KIND=0"],
        "mpsc": ["CAP=3", "NPROD=2", "PROD_OPS=1", "PROD_KIND=1", "CONS_KIND=0", "CONS_OPS=2"],
    }
    for tag, macro in (("seq-store-release-relaxed", "O_SEQ_STORE=memory_order_relaxed"),
                       ("seq-load-acquire-relaxed", "O_SEQ_LOAD=memory_order_relaxed")):
        for k, d in dk.items():
            arm_raw("design-mut-%s@%s" % (tag, k), "scenario.c", d + ["RING_SEQ", macro], "PASS" if k in ("mp2x", "mc2x") else "FAIL", "design-mutant",
                    "a weakened order of the design must be caught where a thread reads what another wrote (spin pass on: a FAIL is a counter-example, a PASS is weaker evidence)", [])
    arm_raw("design-mut-ticket-cas-relaxed-is-enough@mpsc", "scenario.c", dk["mpsc"] + ["RING_SEQ", "O_TICKET_CAS=memory_order_relaxed"], "PASS",
            "design-mutant", "control: the ticket compare-and-swap needs no order (the number carries it) (spin pass on: weaker evidence)", [])

    # ------------------------------------------------------------------------------------------------------
    # Is a header-exact counter wrap reachable with more pushes? (data-race detection OFF: only the end state)
    # ------------------------------------------------------------------------------------------------------
    arm_raw("mptest-both-done-cap1-norace-accounting", "mp_test.c", ["CAP=1", "WAIT_BOTH=1", "WHAT=1"], "PASS", "library-test",
            "two pushes per producer into one slot: the end state satisfies count == pushes - pops - evictions <= capacity "
            "(three pushes per producer did not finish in 10 minutes: reduced)", ["--disable-race-detection"])

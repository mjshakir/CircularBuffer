#!/usr/bin/env python3
"""Runs every GenMC arm of the CircularBuffer verification and prints one line per arm:

    name | expected | actual | executions | seconds

It exits non-zero if any arm's actual verdict differs from its expected verdict, or if an arm was cut off
(timeout, memory guard). Python 3 standard library only.

    python3 run.py                 run every arm
    python3 run.py --list          list the arms
    python3 run.py --only seq      run the arms whose name contains "seq" (repeatable)
    python3 run.py --table         print the fidelity table (model line -> header line)
    python3 run.py --resume LOG    skip the arms already listed in the log of an interrupted run

Options: --timeout S (per arm, default 600), --max-rss-mb N (default 1000), --min-avail-mb N (default 1500),
--nice N (default 19), --genmc PATH, --clang PATH, --header PATH, -v (print GenMC's tail for every arm).
"""
import argparse
import hashlib
import os
import re
import shutil
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
HEADER_DEFAULT = os.path.normpath(os.path.join(HERE, "..", "..", "include", "CircularBuffer.hpp"))

# The header the models were transcribed from. If the header changes, the fidelity arm fails until the
# transcription is checked again and this value is updated.
HEADER_SHA256 = "bc408e38ccb1f33a73a98c8009a8abccdd2735e621d3126be4288a04553725d8"

# The header's concurrent functions: (name, first line, last line). Statistics, constructors, copy, move and
# iterators are single-threaded and are not modelled (see README).
CONCURRENT_RANGES = [
    ("try_emplace", 693, 704),
    ("consume", 727, 738),
    ("insert", 1272, 1289),
    ("read_insert", 1291, 1313),
    ("push_back(const T&)", 1315, 1329),
    ("push_back(T&&)", 1331, 1349),
    ("emplace_back", 1351, 1370),
    ("get_top", 1372, 1380),
    ("get_last", 1382, 1399),
    ("get_top_pop", 1401, 1423),
    ("pop_front", 1425, 1445),
    ("is_empty", 1447, 1451),
    ("full", 1453, 1459),
    ("get_size", 1461, 1465),
    ("get_capacity", 1467, 1475),
    ("clear", 1487, 1497),
    ("increment", 1499, 1506),
    ("pop_front_unsafe", 1756, 1782),
]
# Header lines inside those ranges that are not concurrent logic of the ring: the statistics hooks for
# arithmetic T (atomic_add / atomic_sub on m_sum and m_sum_squares, relaxed, separate atomics), see README.
NOT_MODELLED = {
    1324: "atomic_add(item): statistics of arithmetic T",
    1343: "atomic_add(...): statistics of arithmetic T",
    1364: "atomic_add(...): statistics of arithmetic T",
    1415: "atomic_sub(value): statistics of arithmetic T",
    1435: "atomic_sub(...): statistics of arithmetic T",
    1772: "atomic_sub(...): statistics of arithmetic T",
    1323: "if constexpr (is_arithmetic): statistics of arithmetic T",
    1341: "if constexpr (is_arithmetic): statistics of arithmetic T",
    1362: "if constexpr (is_arithmetic): statistics of arithmetic T",
    1413: "if constexpr (is_arithmetic): statistics of arithmetic T",
    1433: "if constexpr (is_arithmetic): statistics of arithmetic T",
    1770: "if constexpr (is_arithmetic): statistics of arithmetic T",
}
INTERESTING = re.compile(r"m_head|m_tail|m_count|operator\[\]|compare_exchange|memory_order|~T|new \(|while|do \{|return|if \(|if constexpr|break")

GENMC_BASE = ["--rc11", "--disable-estimation"]

# ----------------------------------------------------------------------------------------------------------
# The arms. Fields: name, source file, -D list, extra GenMC flags, expected ("PASS" or "FAIL"), group, note.
# Expectations were written from a reading of the header (see README, "predictions") before the runs.
# ----------------------------------------------------------------------------------------------------------
ARMS = []


def arm(name, src, defs, expect, group, note="", flags=None):
    ARMS.append(dict(name=name, src=src, defs=defs, expect=expect, group=group, note=note, flags=flags or []))


sys.path.insert(0, HERE)
import arms  # the arm table lives in arms.py

arms.define(arm)


# ----------------------------------------------------------------------------------------------------------
def meminfo_available_mb():
    with open("/proc/meminfo") as handle:
        for line in handle:
            if line.startswith("MemAvailable:"):
                return int(line.split()[1]) // 1024
    return 10 ** 9


def tree_rss_mb(root_pid):
    """Resident size in MB of a process and all its descendants."""
    children = {}
    rss = {}
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/stat" % entry) as handle:
                stat = handle.read()
            parent = int(stat[stat.rindex(")") + 2:].split()[1])
            with open("/proc/%s/statm" % entry) as handle:
                pages = int(handle.read().split()[1])
        except (OSError, ValueError, IndexError):
            continue
        children.setdefault(parent, []).append(int(entry))
        rss[int(entry)] = pages * os.sysconf("SC_PAGE_SIZE")
    total, stack = 0, [root_pid]
    while stack:
        pid = stack.pop()
        total += rss.get(pid, 0)
        stack.extend(children.get(pid, []))
    return total // (1024 * 1024)


def resource_dir(clang):
    out = subprocess.run([clang, "-print-resource-dir"], capture_output=True, text=True, check=True)
    return out.stdout.strip()


def run_genmc(a, opts, resdir):
    cmd = ["nice", "-n", str(opts.nice), opts.genmc] + GENMC_BASE + a["flags"] + [
        "--", "-nostdinc", "-isystem", os.path.join(resdir, "include")] + ["-D" + d for d in a["defs"]] + [
        os.path.join(HERE, a["src"])]
    start = time.time()
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, start_new_session=True)
    peak = 0
    cut = None
    # read the output on a thread-free loop: poll the guards while the process runs, collect output at the end
    import threading
    chunks = []

    def pump():
        for line in proc.stdout:
            chunks.append(line)

    reader = threading.Thread(target=pump, daemon=True)
    reader.start()
    while proc.poll() is None:
        time.sleep(0.5)
        rss = tree_rss_mb(proc.pid)
        peak = max(peak, rss)
        if rss > opts.max_rss_mb:
            cut = "MEMKILL(rss %d MB)" % rss
        elif meminfo_available_mb() < opts.min_avail_mb:
            cut = "MEMKILL(avail %d MB)" % meminfo_available_mb()
        elif time.time() - start > opts.timeout:
            cut = "TIMEOUT"
        if cut:
            os.killpg(proc.pid, signal.SIGKILL)
            break
    proc.wait()
    reader.join(5)
    return "".join(chunks), cut, time.time() - start, peak


def verdict(text, cut):
    if cut:
        return cut, "-"
    execs = re.search(r"Number of complete executions explored:\s*(\d+)", text)
    blocked = re.search(r"Number of blocked executions seen:\s*(\d+)", text)
    count = execs.group(1) if execs else "-"
    if blocked and blocked.group(1) != "0":
        count += "+%sb" % blocked.group(1)
    if "No errors were detected" in text:
        return "PASS", count
    if "Non-atomic race" in text or "Non-atomic" in text and "race" in text:
        return "FAIL(race)", count
    if "Assertion violation" in text:
        return "FAIL(assert)", count
    if "Liveness violation" in text or "liveness" in text.lower() and "violation" in text.lower():
        return "FAIL(liveness)", count
    if "Error detected" in text or "error" in text.lower():
        return "FAIL(other)", count
    return "UNKNOWN", count


def matches(expect, actual):
    if expect == "PASS":
        return actual == "PASS"
    if expect == "FAIL":
        return actual.startswith("FAIL")
    return actual == expect or actual.startswith(expect)


# ----------------------------------------------------------------------------------------------------------
# Arms that are not GenMC runs
# ----------------------------------------------------------------------------------------------------------
def header_lines(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read().split("\n")


def scan_model_tags():
    """Every `hpp:<line> "<needle>"` tag and every SITE marker of ring.h."""
    tags, sites = [], []
    with open(os.path.join(HERE, "ring.h"), encoding="utf-8") as handle:
        lines = handle.read().split("\n")
    for number, text in enumerate(lines, 1):
        for match in re.finditer(r'hpp:(\d+)\s+"((?:[^"\\]|\\.)*)"', text):
            tags.append((number, int(match.group(1)), match.group(2), text.strip()))
        site = re.search(r"/\*\s*SITE\s+(\w+)\s+hpp:(\d+)\s+(\w+)\s*\*/", text)
        if site:
            define = lines[number].strip() if number < len(lines) else ""
            sites.append((number, site.group(1), int(site.group(2)), site.group(3), define))
    return tags, sites


def check_fidelity(header_path):
    """Returns (ok, messages). Mechanical: hash, tag needles, memory-order coverage, statement coverage."""
    problems = []
    with open(header_path, "rb") as handle:
        digest = hashlib.sha256(handle.read()).hexdigest()
    if digest != HEADER_SHA256:
        problems.append("header sha256 %s differs from the transcribed %s" % (digest, HEADER_SHA256))
    header = header_lines(header_path)
    tags, sites = scan_model_tags()
    for number, line, needle, _ in tags:
        if line > len(header) or needle.replace('\\"', '"') not in header[line - 1]:
            problems.append("ring.h:%d tag hpp:%d %r not found on that header line" % (number, line, needle))
    # every memory_order occurrence of the concurrent functions has a SITE, with the same order and line
    site_keys = {(line, order) for _, _, line, order, _ in sites}
    for _, first, last in [(n, f, l) for n, f, l in CONCURRENT_RANGES]:
        for line in range(first, last + 1):
            for match in re.finditer(r"std::memory_order_(\w+)", header[line - 1]):
                if (line, match.group(1)) not in site_keys:
                    problems.append("header line %d memory_order_%s has no SITE in ring.h" % (line, match.group(1)))
    for number, name, line, order, define in sites:
        if "std::memory_order_%s" % order not in header[line - 1]:
            problems.append("ring.h:%d SITE %s says hpp:%d %s but the header line has no such order" % (number, name, line, order))
    text = open(os.path.join(HERE, "ring.h"), encoding="utf-8").read()
    for number, name, line, order, define in sites:
        m = re.search(r"#ifndef %s\n#define %s (memory_order_\w+)" % (name, name), text)
        if not m or m.group(1) != "memory_order_" + order:
            problems.append("SITE %s: the default is %s, the header has %s" % (name, m.group(1) if m else "?", order))
    # statement coverage: every interesting header line of a concurrent function is tagged, or listed as not modelled
    tagged = {line for _, line, _, _ in tags} | {line for _, _, line, _, _ in sites}
    for name, first, last in CONCURRENT_RANGES:
        for line in range(first, last + 1):
            code = header[line - 1].split("//")[0]
            if not INTERESTING.search(code) or line in tagged or line in NOT_MODELLED:
                continue
            if re.match(r"\s*(//|\*|/\*)", header[line - 1]) or not code.strip():
                continue
            # a line that opens a function signature or only closes a brace carries nothing
            if re.match(r"\s*(\}|\{)", code) and not re.search(r"while|if|return", code):
                continue
            problems.append("header %s line %d has no model line: %s" % (name, line, header[line - 1].strip()))
    return problems


def print_table(header_path):
    header = header_lines(header_path)
    tags, sites = scan_model_tags()
    print("| model line | header line | memory order | header text |")
    print("|---|---|---|---|")
    order_of = {}
    for _, name, line, order, _ in sites:
        order_of.setdefault(line, []).append(order)
    for number, line, needle, _ in sorted(tags, key=lambda t: (t[1], t[0])):
        order = "/".join(order_of.get(line, [])) or "plain"
        print("| ring.h:%d | %d | %s | `%s` |" % (number, line, order, header[line - 1].strip().replace("|", "\\|")[:110]))
    print()
    print("| header line | memory order | model macro | model line |")
    print("|---|---|---|---|")
    for number, name, line, order, _ in sorted(sites, key=lambda s: s[2]):
        print("| %d | %s | %s | ring.h:%d |" % (line, order, name, number))


BLOCKING = re.compile(r"mutex|lock|condition_variable|condvar|wait|sleep|yield|semaphore|futex|spin|barrier|latch|thread", re.I)


def text_search(header_path):
    """Every hit of a blocking-primitive search in the header, classified. Returns (ok, lines)."""
    header = header_lines(header_path)
    hits, bad = [], []
    for number, text in enumerate(header, 1):
        for match in BLOCKING.finditer(text):
            word = match.group(0)
            stripped = text.strip()
            is_comment = stripped.startswith("*") or stripped.startswith("//") or stripped.startswith("/*")
            in_name = word.lower() == "lock" and "Lock-free" in text
            in_macro = "CIRCULARBUFFER_MULTI_THREADING" in text
            if is_comment or in_macro or "//" in text and text.index("//") < match.start():
                kind = "comment/macro"
            else:
                kind = "CODE"
                bad.append((number, stripped))
            hits.append((number, word, kind, stripped[:100]))
    return hits, bad


# ----------------------------------------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--only", action="append", default=[])
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--table", action="store_true")
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--max-rss-mb", type=int, default=1000)
    parser.add_argument("--min-avail-mb", type=int, default=1500)
    parser.add_argument("--nice", type=int, default=19)
    parser.add_argument("--genmc", default=shutil.which("genmc") or "genmc")
    parser.add_argument("--clang", default=shutil.which("clang") or "clang")
    parser.add_argument("--header", default=HEADER_DEFAULT)
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--resume", help="a log of an earlier run: arms already listed in it are skipped")
    opts = parser.parse_args()

    if opts.table:
        print_table(opts.header)
        return 0
    chosen = [a for a in ARMS if not opts.only or any(o in a["name"] for o in opts.only)]
    if opts.resume:
        with open(opts.resume) as handle:
            done = {line.split("|")[0].strip() for line in handle if "|" in line}
        chosen = [a for a in chosen if a["name"] not in done]
    if opts.list:
        for a in chosen:
            print("%-34s %-5s %-12s %s %s" % (a["name"], a["expect"], a["group"], a["src"], " ".join(a["defs"] + a["flags"])))
        return 0

    resdir = resource_dir(opts.clang)
    mismatches = 0
    print("%-34s | %-8s | %-14s | %-12s | %s" % ("arm", "expected", "actual", "executions", "seconds"))

    def want(name):
        return not opts.only or any(o in name for o in opts.only)

    if want("fidelity"):
        started = time.time()
        problems = check_fidelity(opts.header)
        actual = "PASS" if not problems else "FAIL"
        print("%-34s | %-8s | %-14s | %-12s | %.1f" % ("fidelity", "PASS", actual, "-", time.time() - started))
        for p in problems:
            print("    " + p)
        mismatches += bool(problems)
    if want("text-search"):
        started = time.time()
        hits, bad = text_search(opts.header)
        actual = "PASS" if not bad else "FAIL"
        print("%-34s | %-8s | %-14s | %-12s | %.1f" % ("text-search", "PASS", actual, "%d hits" % len(hits), time.time() - started))
        if opts.verbose or bad:
            for number, word, kind, text in hits:
                print("    %5d %-14s %-14s %s" % (number, word, kind, text))
        mismatches += bool(bad)

    for a in chosen:
        text, cut, seconds, peak = run_genmc(a, opts, resdir)
        actual, count = verdict(text, cut)
        ok = matches(a["expect"], actual)
        print("%-34s | %-8s | %-14s | %-12s | %.1f%s" % (a["name"], a["expect"], actual, count, seconds,
                                                        "" if ok else "   <-- MISMATCH"))
        sys.stdout.flush()
        if opts.verbose or not ok:
            tail = [l for l in text.strip().split("\n") if l.strip()][-14:]
            for l in tail:
                print("    " + l[:200])
        mismatches += not ok
    print("mismatches: %d" % mismatches)
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())

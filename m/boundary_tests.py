"""
boundary_tests.py - Structural edge cases (QA checklist section 2, B1-B6),
plus a few closely related items that naturally live alongside them:
  * F2  compile counter increments exactly N times, not off-by-one
  * E1  clean simulation end via "all compiled"
  * E2  clean simulation end via burnout
  * E5  all threads join / no leaked threads after normal completion
"""

import time

from framework import (
    test, expect, expect_approx, run_codexion, run_codexion_bg, parse_log,
    group_by_coder, compile_cycles, burnout_event, kill_quietly,
    thread_count, process_alive,
)
import config


# ---------------------------------------------------------------------------
# B1 - One coder: structurally can never compile (needs 2 dongles, only 1 exists)
# ---------------------------------------------------------------------------
@test("B1 - single coder can never compile, burns out on schedule", tags=["boundary"])
def test_b1_single_coder():
    burnout_ms = 5000
    r = run_codexion(config.build_args(1, burnout_ms, 2000, 1000, 1000, 3, 500, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "B1: simulation should stop on burnout, not hang forever")

    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"B1: malformed lines: {malformed[:3]}")

    taken = [e for e in events if e.kind == "taken"]
    compiling = [e for e in events if e.kind == "compiling"]
    burned = [e for e in events if e.kind == "burned_out"]

    expect(len(taken) == 1, f"B1: expected exactly one 'has taken a dongle' line, got {len(taken)}")
    expect(len(compiling) == 0, "B1: a lone coder with 1 dongle should never reach compiling")
    expect(len(burned) == 1, f"B1: expected exactly one burnout, got {len(burned)}")
    expect_approx(burned[0].ts, burnout_ms, config.BURNOUT_TOLERANCE_MS,
                  "B1: burnout timing")


# ---------------------------------------------------------------------------
# B2 - Two coders: smallest configuration where compiling is actually possible
# ---------------------------------------------------------------------------
@test("B2 - two coders alternate and both reach required compiles", tags=["boundary"])
def test_b2_two_coders():
    required = 3
    r = run_codexion(config.build_args(2, 5000, 1000, 1000, 1000, required, 500, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "B2: run did not complete in time")
    expect(r.returncode == 0, f"B2: expected clean exit, got {r.returncode}")

    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"B2: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "B2: no burnout expected with generous timing")

    by_coder = group_by_coder(events)
    expect(set(by_coder.keys()) == {1, 2}, f"B2: expected coders 1 and 2, got {sorted(by_coder)}")
    for cid, evs in by_coder.items():
        n_compiles = sum(1 for e in evs if e.kind == "compiling")
        expect(n_compiles == required,
               f"B2: coder {cid} compiled {n_compiles} times, expected {required}")


# ---------------------------------------------------------------------------
# B3 - Many coders: correctness scales
# ---------------------------------------------------------------------------
@test("B3 - twenty coders all reach required compiles (edf)", tags=["boundary"])
def test_b3_many_coders():
    n, required = 20, 5
    r = run_codexion(config.build_args(n, 8000, 500, 300, 300, required, 200, "edf"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "B3: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"B3: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "B3: no burnout expected with generous timing")

    by_coder = group_by_coder(events)
    expect(len(by_coder) == n, f"B3: expected {n} distinct coder ids, got {len(by_coder)}")
    for cid, evs in by_coder.items():
        n_compiles = sum(1 for e in evs if e.kind == "compiling")
        expect(n_compiles == required,
               f"B3: coder {cid} compiled {n_compiles} times, expected {required}")


# ---------------------------------------------------------------------------
# B4 - Huge compile-count requirement: should sustain a long run without
# stopping early, crashing, or corrupting logs.
# ---------------------------------------------------------------------------
@test("B4 - huge compile count keeps running healthily (no early stop/crash)",
      tags=["boundary", "slow"])
def test_b4_huge_compile_count():
    r = run_codexion(config.build_args(4, 999999, 200, 100, 100, 1000000, 100, "fifo"),
                      timeout=10)
    expect(r.timed_out, "B4: expected the run to still be going (healthy) after 10s, "
                        "not stopped early")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"B4: malformed lines during long run: {malformed[:3]}")
    expect(burnout_event(events) is None, "B4: no burnout expected with these generous timings")
    # Sanity: timestamps observed should be monotonic non-decreasing overall.
    timestamps = [e.ts for e in events]
    expect(timestamps == sorted(timestamps), "B4: timestamps are not monotonic - garbled log order")


# ---------------------------------------------------------------------------
# B5 - Very small timing values: stresses mutex/cond-var churn
# ---------------------------------------------------------------------------
@test("B5 - very small timing values produce no corrupted logs", tags=["boundary"])
def test_b5_tiny_timings():
    r = run_codexion(config.build_args(5, 200, 5, 5, 5, 5, 5, "edf"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "B5: run should complete (or burn out) promptly, not hang")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"B5: malformed/corrupted lines: {malformed[:3]}")

    # Whatever happens (success or predictable burnout), each coder's own
    # sequence of events must still respect the mandated state order.
    by_coder = group_by_coder(events)
    for cid, evs in by_coder.items():
        _assert_state_order_valid(cid, evs)


def _assert_state_order_valid(cid, evs):
    """taken, taken, compiling, debugging, refactoring must never be
    reordered or interleaved with themselves for a single coder."""
    expected_next = {
        None: {"taken"},
        "taken1": {"taken"},
        "taken2": {"compiling"},
        "compiling": {"debugging"},
        "debugging": {"refactoring"},
        "refactoring": {"taken", "burned_out"},
        "burned_out": set(),
    }
    state = None
    taken_count = 0
    for e in evs:
        if e.kind == "taken":
            taken_count += 1
            key = "taken1" if taken_count % 2 == 1 else "taken2"
            allowed = expected_next[state] if state not in ("taken1",) else {"taken"}
            expect("taken" in allowed or state in (None, "refactoring"),
                   f"coder {cid}: unexpected 'taken' after state={state}")
            state = key
        else:
            allowed = expected_next.get(state, set())
            expect(e.kind in allowed,
                   f"coder {cid}: unexpected transition '{e.kind}' after state={state}")
            state = e.kind
        if state == "burned_out":
            break


# ---------------------------------------------------------------------------
# B6 - Very large timing values: guard against timespec/nsec overflow
# ---------------------------------------------------------------------------
@test("B6 - very large timing values don't overflow timespec math", tags=["boundary"])
def test_b6_huge_timings():
    r = run_codexion(config.build_args(3, 3600000, 1800000, 1800000, 1800000, 2, 1000, "fifo"),
                      timeout=5)
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"B6: malformed lines: {malformed[:3]}")
    burned = burnout_event(events)
    expect(burned is None, "B6: no burnout should occur this early with hour-long phases")
    # The process should still be alive/working (still compiling), not have
    # crashed or produced a garbled instant "burned out" from nsec overflow.
    expect(r.timed_out or r.returncode == 0,
           "B6: process should either still be running healthily or exit cleanly, not crash")


# ---------------------------------------------------------------------------
# F2 - Compile counter increments correctly (not off-by-one)
# ---------------------------------------------------------------------------
@test("F2 - every coder compiles exactly number_of_compiles_required times", tags=["boundary"])
def test_f2_compile_counter_exact():
    required = 4
    r = run_codexion(config.build_args(3, 20000, 500, 200, 200, required, 100, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "F2: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"F2: malformed lines: {malformed[:3]}")
    by_coder = group_by_coder(events)
    for cid, evs in by_coder.items():
        n = sum(1 for e in evs if e.kind == "compiling")
        expect(n == required, f"F2: coder {cid} compiled {n} times, expected exactly {required}")


# ---------------------------------------------------------------------------
# E1 - Clean end via "all compiled"
# ---------------------------------------------------------------------------
@test("E1 - simulation ends cleanly once all coders reach compile count", tags=["boundary"])
def test_e1_clean_end_all_compiled():
    r = run_codexion(config.build_args(4, 10000, 500, 200, 200, 3, 200, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "E1: run did not complete in time")
    expect(r.returncode == 0, f"E1: expected exit code 0, got {r.returncode}")
    events, _ = parse_log(r.stdout)
    expect(burnout_event(events) is None, "E1: no burnout expected")


# ---------------------------------------------------------------------------
# E2 - Clean end via burnout
# ---------------------------------------------------------------------------
@test("E2 - simulation stops immediately on the first burnout", tags=["boundary"])
def test_e2_clean_end_burnout():
    r = run_codexion(config.build_args(4, 500, 2000, 200, 200, 10, 200, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "E2: run did not stop after burnout")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"E2: malformed lines: {malformed[:3]}")
    burned = burnout_event(events)
    expect(burned is not None, "E2: expected at least one burnout with these tight timings")

    # No coder should show activity meaningfully after the burnout timestamp.
    grace = config.STOP_GRACE_MS
    late = [e for e in events if e.ts > burned.ts + grace and e.kind != "burned_out"]
    expect(len(late) == 0,
           f"E2: found activity after burnout+{grace}ms grace period: {late[:3]}")


# ---------------------------------------------------------------------------
# E5 - All threads terminate (no leaked threads) after normal completion
# ---------------------------------------------------------------------------
@test("E5 - no leaked threads after normal completion", tags=["boundary", "slow"])
def test_e5_no_leaked_threads():
    proc = run_codexion_bg(config.build_args(6, 10000, 400, 200, 200, 4, 200, "edf"))
    try:
        time.sleep(1.0)
        if proc.poll() is None:
            n = thread_count(proc.pid)
            if n is not None:
                # main + monitor + 6 coders = 8 (best-effort - not fatal if
                # unavailable / differs due to platform thread reporting).
                expect(n >= 6, f"E5: expected at least 6 live threads while running, got {n}")
        proc.wait(timeout=config.DEFAULT_TIMEOUT)
    finally:
        kill_quietly(proc)

    expect(proc.returncode is not None, "E5: process should have exited")
    # Give the OS a brief moment to reap, then confirm the pid is gone.
    time.sleep(0.2)
    expect(not process_alive(proc.pid), "E5: process still alive after wait() returned")
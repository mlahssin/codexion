"""
timing_tests.py - Phase-duration accuracy and burnout precision
(QA checklist section 7, T1/T2/T4/T5), plus F1 (full per-coder lifecycle
order & timing, the canonical "does the state machine match the subject"
test).
"""

from framework import (
    test, expect, expect_approx, run_codexion, parse_log, group_by_coder,
    compile_cycles, burnout_event,
)
import config


# ---------------------------------------------------------------------------
# F1 - Full lifecycle order & timing for a coder (with a partner so compiling
# is actually reachable).
# ---------------------------------------------------------------------------
@test("F1 - full per-cycle state order and timing matches the subject", tags=["timing"])
def test_f1_full_lifecycle():
    time_to_compile, time_to_debug, time_to_refactor = 1000, 500, 500
    r = run_codexion(
        config.build_args(2, 10000, time_to_compile, time_to_debug, time_to_refactor,
                           1, 500, "fifo"),
        timeout=config.DEFAULT_TIMEOUT,
    )
    expect(not r.timed_out, "F1: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"F1: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "F1: no burnout expected")

    by_coder = group_by_coder(events)
    for cid, evs in by_coder.items():
        taken = [e.ts for e in evs if e.kind == "taken"]
        expect(len(taken) == 2, f"F1: coder {cid} should have exactly 2 dongle-taken lines, "
                                 f"got {len(taken)}")
        cycles = compile_cycles(evs)
        expect(len(cycles) == 1, f"F1: coder {cid} expected exactly 1 cycle, got {len(cycles)}")
        c = cycles[0]
        expect(c.compile_ts is not None, f"F1: coder {cid} missing compile timestamp")
        expect(c.debug_ts is not None, f"F1: coder {cid} missing debug timestamp")
        expect(c.refactor_ts is not None, f"F1: coder {cid} missing refactor timestamp")

        # Compiling must start right at/after the 2nd dongle acquisition.
        expect_approx(c.compile_ts, c.dongle_ts[-1], config.PHASE_TOLERANCE_MS,
                      f"F1: coder {cid} compiling should start right after 2nd dongle taken")
        expect_approx(c.debug_ts - c.compile_ts, time_to_compile, config.PHASE_TOLERANCE_MS,
                      f"F1: coder {cid} compile phase duration")
        expect_approx(c.refactor_ts - c.debug_ts, time_to_debug, config.PHASE_TOLERANCE_MS,
                      f"F1: coder {cid} debug phase duration")


# ---------------------------------------------------------------------------
# T1 - Compile phase duration accuracy across a full run
# ---------------------------------------------------------------------------
@test("T1 - compile phase duration matches time_to_compile within tolerance", tags=["timing"])
def test_t1_compile_duration():
    time_to_compile = 1500
    r = run_codexion(config.build_args(2, 20000, time_to_compile, 500, 500, 3, 300, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "T1: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"T1: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "T1: no burnout expected")

    by_coder = group_by_coder(events)
    checked = 0
    for cid, evs in by_coder.items():
        for cycle in compile_cycles(evs):
            if cycle.compile_ts is not None and cycle.debug_ts is not None:
                expect_approx(cycle.debug_ts - cycle.compile_ts, time_to_compile,
                              config.PHASE_TOLERANCE_MS,
                              f"T1: coder {cid} compile duration")
                checked += 1
    expect(checked > 0, "T1: no complete compile cycles observed to check")


# ---------------------------------------------------------------------------
# T2 - Debug and refactor phase duration accuracy
# ---------------------------------------------------------------------------
@test("T2 - debug and refactor phase durations match configured values", tags=["timing"])
def test_t2_debug_refactor_duration():
    time_to_debug, time_to_refactor = 1200, 900
    r = run_codexion(config.build_args(2, 20000, 500, time_to_debug, time_to_refactor,
                                        3, 300, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "T2: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"T2: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "T2: no burnout expected")

    by_coder = group_by_coder(events)
    checked_debug = checked_refactor = 0
    for cid, evs in by_coder.items():
        cycles = compile_cycles(evs)
        for i, cycle in enumerate(cycles):
            if cycle.debug_ts is not None and cycle.refactor_ts is not None:
                expect_approx(cycle.refactor_ts - cycle.debug_ts, time_to_debug,
                              config.PHASE_TOLERANCE_MS, f"T2: coder {cid} debug duration")
                checked_debug += 1
            # refactor ends when the *next* cycle's compiling begins (right
            # after the coder re-acquires both dongles).
            if cycle.refactor_ts is not None and i + 1 < len(cycles):
                nxt = cycles[i + 1]
                if nxt.compile_ts is not None:
                    gap = nxt.compile_ts - cycle.refactor_ts
                    # Can only be exactly time_to_refactor if dongles were
                    # immediately available; otherwise it's >= time_to_refactor.
                    expect(gap >= time_to_refactor - config.PHASE_TOLERANCE_MS,
                           f"T2: coder {cid} refactor phase shorter than configured "
                           f"({gap}ms < {time_to_refactor}ms)")
                    checked_refactor += 1
    expect(checked_debug > 0, "T2: no debug phases observed to check")


# ---------------------------------------------------------------------------
# T4 - Burnout timing precision (hard requirement: within 10ms)
# ---------------------------------------------------------------------------
@test("T4 - burnout is logged within 10ms of the actual deadline", tags=["timing"])
def test_t4_burnout_precision():
    time_to_burnout = 1000
    r = run_codexion(config.build_args(3, time_to_burnout, 2000, 100, 100, 5, 100, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "T4: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"T4: malformed lines: {malformed[:3]}")

    burned = burnout_event(events)
    expect(burned is not None, "T4: expected a burnout with these parameters "
                                "(time_to_compile > time_to_burnout)")

    by_coder = group_by_coder(events)
    coder_events = by_coder[burned.coder]
    # Deadline = last compile start (or simulation start, i.e. 0) + burnout window.
    last_compile_start = 0
    for e in coder_events:
        if e.ts >= burned.ts:
            break
        if e.kind == "compiling":
            last_compile_start = e.ts

    expected_burnout_ts = last_compile_start + time_to_burnout
    expect_approx(burned.ts, expected_burnout_ts, config.BURNOUT_TOLERANCE_MS,
                  f"T4: burnout precision for coder {burned.coder}")


# ---------------------------------------------------------------------------
# T5 - Burnout timer resets after every successful compile
# ---------------------------------------------------------------------------
@test("T5 - burnout timer resets on each compile; no burnout across many cycles",
      tags=["timing"])
def test_t5_burnout_resets():
    r = run_codexion(config.build_args(2, 3000, 1000, 500, 500, 10, 200, "fifo"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "T5: run did not complete in time")
    expect(r.returncode == 0, f"T5: expected clean exit, got {r.returncode}")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"T5: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None,
           "T5: burnout timer should reset on every compile; no burnout expected")

    by_coder = group_by_coder(events)
    for cid, evs in by_coder.items():
        n = sum(1 for e in evs if e.kind == "compiling")
        expect(n == 10, f"T5: coder {cid} completed {n} compiles, expected 10")
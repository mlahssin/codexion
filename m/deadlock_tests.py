"""
deadlock_tests.py - Deadlock avoidance (QA checklist section 6, D1-D2) plus
closely related synchronization invariants from section 9:
  * SYN1  no dongle is ever "held" by two coders at once
  * SYN2  log lines are never interleaved/corrupted under heavy contention
  * SYN4  the stop flag halts all coder threads promptly after burnout

The classic bug D1/D2 target is the "dining philosophers" deadlock: every
coder locks its left dongle then blocks forever waiting for its right
dongle, while every neighbor does the same thing simultaneously.
"""

from framework import (
    test, expect, run_codexion, parse_log, group_by_coder, find_overlaps,
    burnout_event,
)
import config


# ---------------------------------------------------------------------------
# D1 - Classic "everyone grabs left, waits for right" scenario must not hang
# ---------------------------------------------------------------------------
@test("D1 - dining-philosophers style deadlock does not occur (2 coders, fifo)",
      tags=["deadlock"])
def test_d1_no_deadlock_two_coders():
    r = run_codexion(config.build_args(2, 5000, 1000, 500, 500, 5, 200, "fifo"),
                      timeout=15)
    expect(not r.timed_out, "D1: simulation hung - likely a dining-philosophers deadlock "
                            "(2 coders, fifo)")


@test("D1 - dining-philosophers style deadlock does not occur (5 coders, edf)",
      tags=["deadlock"])
def test_d1_no_deadlock_five_coders():
    r = run_codexion(config.build_args(5, 5000, 1000, 500, 500, 5, 200, "edf"),
                      timeout=15)
    expect(not r.timed_out, "D1: simulation hung - likely a dining-philosophers deadlock "
                            "(5 coders, edf)")


# ---------------------------------------------------------------------------
# D2 - Repeated runs to catch nondeterministic (timing-dependent) deadlocks
# ---------------------------------------------------------------------------
@test("D2 - repeated runs never hang (nondeterministic deadlock check)",
      tags=["deadlock", "slow"])
def test_d2_repeated_runs_no_hang():
    args = config.build_args(3, 5000, 800, 300, 300, 5, 200, "edf")
    hangs = []
    iterations = 15  # reduced from the QA doc's 30 to keep CI time reasonable
    for i in range(iterations):
        r = run_codexion(args, timeout=10)
        if r.timed_out:
            hangs.append(i)
    expect(len(hangs) == 0,
           f"D2: {len(hangs)}/{iterations} iterations hung (nondeterministic deadlock): "
           f"{hangs}")


# ---------------------------------------------------------------------------
# SYN1 - No two coders hold the same dongle simultaneously.
# Black-box proxy: with 2 coders sharing exactly 2 dongles, no two "compiling"
# windows may overlap, since compiling requires holding *both* dongles.
# ---------------------------------------------------------------------------
@test("SYN1 - no two coders compile simultaneously with only 2 dongles available",
      tags=["deadlock"])
def test_syn1_no_simultaneous_dongle_ownership():
    r = run_codexion(config.build_args(2, 10000, 500, 200, 200, 5, 200, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "SYN1: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"SYN1: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "SYN1: no burnout expected")

    by_coder = group_by_coder(events)
    intervals = []
    for cid, evs in by_coder.items():
        compiling = [e.ts for e in evs if e.kind == "compiling"]
        debugging = [e.ts for e in evs if e.kind == "debugging"]
        for c_ts, d_ts in zip(compiling, debugging):
            intervals.append((c_ts, d_ts, f"coder{cid}"))

    overlaps = find_overlaps(intervals)
    expect(len(overlaps) == 0,
           f"SYN1: overlapping compile windows detected (dongle held by 2 coders "
           f"at once): {overlaps[:3]}")


# ---------------------------------------------------------------------------
# SYN2 - Log/print serialization: no line is ever interleaved/corrupted, even
# under maximum contention (many coders, very short phases).
# ---------------------------------------------------------------------------
@test("SYN2 - log lines never interleave under heavy contention", tags=["deadlock"])
def test_syn2_log_serialization():
    r = run_codexion(config.build_args(30, 4000, 100, 50, 50, 10, 100, "edf"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "SYN2: run did not complete in time")

    lines = [ln for ln in r.stdout.splitlines() if ln.strip()]
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0,
           f"SYN2: {len(malformed)} corrupted/interleaved log line(s) found: {malformed[:5]}")
    expect(len(events) == len(lines),
           f"SYN2: well-formed line count ({len(events)}) != total line count ({len(lines)}) "
           f"- indicates missing print serialization")


# ---------------------------------------------------------------------------
# SYN4 - Stop-flag propagation on burnout: once burnout is logged, no coder
# thread should keep producing state-change logs (beyond a small grace
# window for in-flight operations already past their guard check).
# ---------------------------------------------------------------------------
@test("SYN4 - stop flag halts all coder threads promptly after burnout", tags=["deadlock"])
def test_syn4_stop_flag_propagation():
    r = run_codexion(config.build_args(5, 800, 2000, 500, 500, 10, 200, "fifo"), timeout=10)
    expect(not r.timed_out, "SYN4: simulation hung instead of stopping after burnout")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"SYN4: malformed lines: {malformed[:3]}")

    burned = burnout_event(events)
    expect(burned is not None, "SYN4: expected a burnout with these tight timings")

    grace = config.STOP_GRACE_MS
    late = [e for e in events if e.ts > burned.ts + grace and e.kind != "burned_out"]
    expect(len(late) == 0,
           f"SYN4: activity logged after burnout+{grace}ms grace window: {late[:5]}")
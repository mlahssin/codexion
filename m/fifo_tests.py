"""
fifo_tests.py - FIFO scheduler correctness (QA checklist section 4, SCH1 & the
fifo half of SCH4), plus F4 (a coder must actually wait when dongles are busy
-- the core mutual-exclusion guarantee that FIFO relies on).

Note on SCH1 (true arrival-order verification): the subject's log format has
no internal "request arrived" timestamp, so we can't prove strict FIFO order
from stdout alone. What we *can* verify externally is that the outcome is
deterministic and well-formed across repeated runs -- i.e. no run produces a
different set/count of events, which would indicate a nondeterministic
(broadcast-and-race) queue implementation instead of a real FIFO structure.
"""

from framework import (
    test, expect, run_codexion, parse_log, group_by_coder, burnout_event,
    find_overlaps,
)
import config


# ---------------------------------------------------------------------------
# SCH1 - FIFO ordering under contention: deterministic across repeated runs
# ---------------------------------------------------------------------------
@test("SCH1 - FIFO produces deterministic, well-formed results across repeated runs",
      tags=["fifo", "slow"])
def test_sch1_fifo_deterministic_repeats():
    args = config.build_args(5, 30000, 300, 100, 100, 5, 200, "fifo")
    signatures = []
    for i in range(5):
        r = run_codexion(args, timeout=config.LONG_TIMEOUT)
        expect(not r.timed_out, f"SCH1: run {i} did not complete in time")
        events, malformed = parse_log(r.stdout)
        expect(len(malformed) == 0, f"SCH1: run {i} malformed lines: {malformed[:3]}")
        expect(burnout_event(events) is None, f"SCH1: run {i} unexpected burnout")

        by_coder = group_by_coder(events)
        expect(len(by_coder) == 5, f"SCH1: run {i} expected 5 coders, got {len(by_coder)}")
        per_coder_compiles = tuple(sorted(
            sum(1 for e in evs if e.kind == "compiling") for evs in by_coder.values()
        ))
        signatures.append(per_coder_compiles)

    expect(all(sig == signatures[0] for sig in signatures),
           f"SCH1: compile-count distribution differs across runs: {signatures}")


# ---------------------------------------------------------------------------
# SCH4 (fifo half) - Scheduler affects arbitration only, not correctness
# ---------------------------------------------------------------------------
@test("SCH4 - fifo scheduling completes correctly with no burnout", tags=["fifo"])
def test_sch4_fifo_correctness():
    r = run_codexion(config.build_args(6, 15000, 500, 200, 200, 4, 200, "fifo"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "SCH4/fifo: did not complete in time")
    expect(r.returncode == 0, f"SCH4/fifo: expected clean exit, got {r.returncode}")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"SCH4/fifo: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "SCH4/fifo: no burnout expected")


# ---------------------------------------------------------------------------
# F4 - A coder must actually wait when the dongles it needs are busy
# (mutual exclusion: no two coders may be "compiling" at overlapping times
# when only 2 dongles exist total, i.e. the 2-coder configuration).
# ---------------------------------------------------------------------------
@test("F4 - coder waits for busy dongles; compiling windows never overlap (2 coders)",
      tags=["fifo"])
def test_f4_mutual_exclusion_two_coders():
    r = run_codexion(config.build_args(2, 10000, 2000, 100, 100, 3, 100, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "F4: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"F4: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "F4: no burnout expected with generous timing")

    by_coder = group_by_coder(events)
    intervals = []
    for cid, evs in by_coder.items():
        compiling_ts = [e.ts for e in evs if e.kind == "compiling"]
        debugging_ts = [e.ts for e in evs if e.kind == "debugging"]
        expect(len(compiling_ts) == len(debugging_ts),
               f"F4: coder {cid} has mismatched compiling/debugging counts")
        for c_ts, d_ts in zip(compiling_ts, debugging_ts):
            intervals.append((c_ts, d_ts, f"coder{cid}"))

    overlaps = find_overlaps(intervals)
    expect(len(overlaps) == 0,
           f"F4: found overlapping compiling windows (mutual exclusion violated): {overlaps[:3]}")
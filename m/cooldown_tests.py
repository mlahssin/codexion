"""
cooldown_tests.py - Dongle cooldown enforcement (QA checklist section 7, T3)
and dongle reuse correctness (F3).

Caveat: the mandated log format identifies coders and event kinds, but not
individual dongle identity. With exactly 2 coders (== 2 dongles), any dongle
released by one coder must be one of the (at most two) dongles the other
coder needs next, so a system-wide check of "next acquisition after a
release must be >= dongle_cooldown later" is a faithful proxy for the
per-dongle rule in that configuration.
"""

from framework import (
    test, expect, run_codexion, parse_log, group_by_coder, compile_cycles,
    burnout_event,
)
import config


# ---------------------------------------------------------------------------
# T3 - Cooldown enforcement
# ---------------------------------------------------------------------------
@test("T3 - dongles are not reacquired before dongle_cooldown has elapsed",
      tags=["cooldown"])
def test_t3_cooldown_enforced():
    cooldown = 1000
    r = run_codexion(config.build_args(2, 20000, 500, 200, 200, 5, cooldown, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "T3: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"T3: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "T3: no burnout expected")

    # Release times: a coder releases both its dongles the instant it starts
    # debugging (per subject: "when a coder finishes compiling, they put both
    # dongles back on the table and start debugging").
    release_events = sorted(
        (e.ts for e in events if e.kind == "debugging")
    )
    acquire_events = sorted(
        (e.ts for e in events if e.kind == "taken")
    )

    expect(len(release_events) > 0, "T3: no debugging events observed")
    expect(len(acquire_events) > 0, "T3: no dongle-taken events observed")

    violations = []
    for release_ts in release_events:
        # Find the very next dongle acquisition anywhere in the system after
        # this release; with only 2 coders/2 dongles it must involve a
        # dongle that was just released.
        nxt = next((a for a in acquire_events if a > release_ts), None)
        if nxt is None:
            continue
        gap = nxt - release_ts
        if gap < cooldown - config.COOLDOWN_TOLERANCE_MS:
            violations.append((release_ts, nxt, gap))

    expect(len(violations) == 0,
           f"T3: dongle(s) reacquired before cooldown elapsed: {violations[:3]}")


# ---------------------------------------------------------------------------
# F3 - Dongles are actually released and reused (no leak/growth, no stuck dongle)
# ---------------------------------------------------------------------------
@test("F3 - dongles are reused correctly across all compile cycles", tags=["cooldown"])
def test_f3_dongle_reuse():
    r = run_codexion(config.build_args(2, 10000, 500, 200, 200, 5, 300, "fifo"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "F3: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"F3: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "F3: no burnout expected")

    total_taken = sum(1 for e in events if e.kind == "taken")
    total_compiling = sum(1 for e in events if e.kind == "compiling")
    expect(total_taken == 2 * total_compiling,
           f"F3: expected exactly 2 dongle-taken lines per compile "
           f"(got {total_taken} taken vs {total_compiling} compiling)")

    by_coder = group_by_coder(events)
    for cid, evs in by_coder.items():
        cycles = compile_cycles(evs)
        expect(len(cycles) == 5, f"F3: coder {cid} completed {len(cycles)} cycles, expected 5")
        for cycle in cycles:
            expect(len(cycle.dongle_ts) == 2,
                   f"F3: coder {cid} cycle acquired {len(cycle.dongle_ts)} dongles, expected 2")
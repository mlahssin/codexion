"""
edf_tests.py - EDF scheduler correctness (QA checklist section 4, SCH2, SCH3,
and the edf half of SCH4).

Like FIFO, the raw log format doesn't expose internal request timestamps or
computed deadlines, so SCH2/SCH3 are checked with the strongest black-box
proxy available: EDF must not let any single coder be starved relative to
others (bounded wait-time spread) and must behave deterministically across
repeated runs with symmetric/tied parameters.
"""

from framework import (
    test, expect, run_codexion, parse_log, group_by_coder, compile_cycles,
    burnout_event,
)
import config


# ---------------------------------------------------------------------------
# SCH2 - EDF grants earliest deadline first (proxy: no coder is left waiting
# disproportionately longer than others across the run).
# ---------------------------------------------------------------------------
@test("SCH2 - EDF keeps per-coder wait times bounded/comparable across coders",
      tags=["edf"])
def test_sch2_edf_bounded_wait_spread():
    r = run_codexion(config.build_args(4, 6000, 1000, 200, 200, 5, 100, "edf"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "SCH2: run did not complete in time")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"SCH2: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "SCH2: no burnout expected with these parameters")

    by_coder = group_by_coder(events)
    completion_ts = {}
    for cid, evs in by_coder.items():
        cycles = compile_cycles(evs)
        expect(len(cycles) == 5, f"SCH2: coder {cid} completed {len(cycles)} cycles, expected 5")
        completion_ts[cid] = cycles[-1].refactor_ts or cycles[-1].compile_ts

    spread = max(completion_ts.values()) - min(completion_ts.values())
    # No hard-and-fast bound exists in the subject, but a coder finishing
    # wildly later than everyone else (many multiples of a full cycle) is a
    # strong smell of starvation-by-scheduling-bug rather than fair EDF.
    single_cycle_ms = 1000 + 200 + 200
    expect(spread < single_cycle_ms * 5,
           f"SCH2: completion spread across coders ({spread}ms) suggests one coder "
           f"was starved relative to the others")


# ---------------------------------------------------------------------------
# SCH3 - EDF tie-breaker determinism
# ---------------------------------------------------------------------------
@test("SCH3 - EDF tie-break resolution is deterministic across repeated runs",
      tags=["edf", "slow"])
def test_sch3_edf_tiebreak_determinism():
    # Symmetric 2-coder setup: both start at t~=0 with identical parameters,
    # maximizing the chance of tied deadlines.
    args = config.build_args(2, 5000, 1000, 500, 500, 5, 500, "edf")
    orders = []
    for i in range(5):
        r = run_codexion(args, timeout=config.DEFAULT_TIMEOUT)
        expect(not r.timed_out, f"SCH3: run {i} did not complete in time")
        events, malformed = parse_log(r.stdout)
        expect(len(malformed) == 0, f"SCH3: run {i} malformed lines: {malformed[:3]}")
        expect(burnout_event(events) is None, f"SCH3: run {i} unexpected burnout")

        compiling_order = [e.coder for e in events if e.kind == "compiling"]
        orders.append(tuple(compiling_order))

    expect(all(o == orders[0] for o in orders),
           f"SCH3: tie-break ordering is not deterministic across runs: {orders}")


# ---------------------------------------------------------------------------
# SCH4 (edf half) - Scheduler affects arbitration only, not correctness
# ---------------------------------------------------------------------------
@test("SCH4 - edf scheduling completes correctly with no burnout", tags=["edf"])
def test_sch4_edf_correctness():
    r = run_codexion(config.build_args(6, 15000, 500, 200, 200, 4, 200, "edf"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "SCH4/edf: did not complete in time")
    expect(r.returncode == 0, f"SCH4/edf: expected clean exit, got {r.returncode}")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"SCH4/edf: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "SCH4/edf: no burnout expected")
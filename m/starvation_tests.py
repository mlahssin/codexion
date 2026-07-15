"""
starvation_tests.py - Liveness/fairness guarantees (QA checklist section 5,
ST1-ST3).

ST1 is a hard subject requirement: "no coder should be starved of dongles and
burn out under edf scheduling, provided the parameters are feasible."
"""

from framework import test, expect, run_codexion, parse_log, group_by_coder, burnout_event
import config


# ---------------------------------------------------------------------------
# ST1 - Liveness guarantee under EDF (subject-mandated, hard requirement)
# ---------------------------------------------------------------------------
@test("ST1 - EDF guarantees liveness under heavy contention (no starvation)",
      tags=["starvation", "slow"])
def test_st1_edf_liveness():
    required = 20
    r = run_codexion(config.build_args(10, 4000, 500, 300, 300, required, 300, "edf"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "ST1: did not finish on its own within the timeout "
                            "(possible starvation/hang)")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"ST1: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None,
           "ST1: EDF must guarantee no coder burns out under feasible parameters")

    by_coder = group_by_coder(events)
    expect(len(by_coder) == 10, f"ST1: expected 10 coders, got {len(by_coder)}")
    for cid, evs in by_coder.items():
        n = sum(1 for e in evs if e.kind == "compiling")
        expect(n == required, f"ST1: coder {cid} compiled {n} times, expected {required} "
                               f"(possible per-coder starvation)")


# ---------------------------------------------------------------------------
# ST2 - FIFO fairness under sustained contention
# ---------------------------------------------------------------------------
@test("ST2 - FIFO prevents starvation under sustained contention", tags=["starvation", "slow"])
def test_st2_fifo_fairness():
    required = 20
    r = run_codexion(config.build_args(10, 8000, 500, 300, 300, required, 300, "fifo"),
                      timeout=config.LONG_TIMEOUT)
    expect(not r.timed_out, "ST2: did not finish on its own within the timeout "
                            "(possible starvation/hang)")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"ST2: malformed lines: {malformed[:3]}")
    expect(burnout_event(events) is None, "ST2: FIFO should not starve any coder here")

    by_coder = group_by_coder(events)
    expect(len(by_coder) == 10, f"ST2: expected 10 coders, got {len(by_coder)}")
    for cid, evs in by_coder.items():
        n = sum(1 for e in evs if e.kind == "compiling")
        expect(n == required, f"ST2: coder {cid} compiled {n} times, expected {required}")


# ---------------------------------------------------------------------------
# ST3 - Deliberately infeasible parameters: burnout must still be detected
# promptly (i.e. the simulation must not falsely "succeed" or hang forever).
# ---------------------------------------------------------------------------
@test("ST3 - infeasible parameters correctly produce a prompt burnout", tags=["starvation"])
def test_st3_infeasible_parameters_burnout():
    r = run_codexion(config.build_args(10, 500, 1000, 500, 500, 5, 2000, "edf"),
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "ST3: simulation hung instead of detecting burnout")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"ST3: malformed lines: {malformed[:3]}")

    burned = burnout_event(events)
    expect(burned is not None,
           "ST3: expected at least one burnout with these infeasible parameters")

    grace = config.STOP_GRACE_MS
    late_compiles = [e for e in events
                     if e.kind == "compiling" and e.ts > burned.ts + grace]
    expect(len(late_compiles) == 0,
           f"ST3: coder(s) still compiling after burnout+{grace}ms: {late_compiles[:3]}")
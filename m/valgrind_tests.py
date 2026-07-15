"""
valgrind_tests.py - Memcheck leak/error detection (QA checklist section 12,
V1-V3).

These tests shell out to `valgrind --tool=memcheck` and are consequently much
slower than the rest of the suite; they are tagged "valgrind" + "slow" so
run_tests.py can skip them by default and only run them on request
(e.g. `python run_tests.py --with-valgrind`).
"""

from framework import test, expect, run_under_valgrind, parse_memcheck_summary
import config

TAGS = ["valgrind", "slow"]


def _assert_clean_memcheck(summary, context):
    expect(summary["definitely_lost"] == 0,
           f"{context}: {summary['definitely_lost']} bytes definitely lost")
    expect(summary["indirectly_lost"] == 0,
           f"{context}: {summary['indirectly_lost']} bytes indirectly lost")
    expect(summary["possibly_lost"] == 0,
           f"{context}: {summary['possibly_lost']} bytes possibly lost")
    expect(summary["invalid_rw"] == 0,
           f"{context}: {summary['invalid_rw']} invalid read/write occurrence(s)")
    expect(summary["errors"] == 0,
           f"{context}: memcheck reported {summary['errors']} error(s)")


# ---------------------------------------------------------------------------
# V1 - Full leak check, normal completion
# ---------------------------------------------------------------------------
@test("V1 - zero memory leaks/errors on normal completion", tags=TAGS)
def test_v1_clean_normal_completion():
    args = config.build_args(4, 8000, 500, 200, 200, 3, 200, "fifo")
    r = run_under_valgrind(args, tool="memcheck")
    expect(not r.timed_out, "V1: valgrind run timed out")
    summary = parse_memcheck_summary(r.stderr)
    _assert_clean_memcheck(summary, "V1")


# ---------------------------------------------------------------------------
# V2 - Leak check on the burnout-triggered early-termination path
# ---------------------------------------------------------------------------
@test("V2 - zero memory leaks/errors on burnout-triggered early exit", tags=TAGS)
def test_v2_clean_burnout_path():
    args = config.build_args(4, 500, 2000, 200, 200, 10, 200, "fifo")
    r = run_under_valgrind(args, tool="memcheck")
    expect(not r.timed_out, "V2: valgrind run timed out")
    summary = parse_memcheck_summary(r.stderr)
    _assert_clean_memcheck(summary, "V2")


# ---------------------------------------------------------------------------
# V3 - Leak check on the invalid-argument rejection path (nothing should leak
# even though the program exits before ever starting the simulation)
# ---------------------------------------------------------------------------
@test("V3 - zero memory leaks/errors on invalid-argument rejection path", tags=TAGS)
def test_v3_clean_rejection_path():
    args = ["-1", "5000", "500", "200", "200", "3", "200", "fifo"]
    r = run_under_valgrind(args, tool="memcheck")
    expect(not r.timed_out, "V3: valgrind run timed out")
    summary = parse_memcheck_summary(r.stderr)
    _assert_clean_memcheck(summary, "V3")
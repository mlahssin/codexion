"""
helgrind_tests.py - Data race / lock-order / cond-var misuse detection
(QA checklist section 11, H1-H2).

Slow (helgrind instrumentation overhead is heavy), so tagged "slow" +
"helgrind" and skipped by default -- run explicitly with
`python run_tests.py --with-valgrind` (or --tags helgrind).
"""

from framework import test, expect, run_under_valgrind, parse_helgrind_summary
import config

TAGS = ["helgrind", "slow"]


def _assert_clean_helgrind(summary, context):
    expect(summary["data_races"] == 0,
           f"{context}: {summary['data_races']} possible data race(s) detected")
    expect(summary["lock_order"] == 0,
           f"{context}: {summary['lock_order']} lock-order violation(s) detected")
    expect(summary["unlocked_not_locked"] == 0,
           f"{context}: {summary['unlocked_not_locked']} unlock-without-lock bug(s) detected")
    expect(summary["errors"] == 0,
           f"{context}: helgrind reported {summary['errors']} error(s)")


# ---------------------------------------------------------------------------
# H1 - Clean run under Helgrind (baseline contention)
# ---------------------------------------------------------------------------
@test("H1 - zero helgrind errors on a baseline run", tags=TAGS)
def test_h1_clean_helgrind_baseline():
    args = config.build_args(4, 8000, 500, 200, 200, 3, 200, "fifo")
    r = run_under_valgrind(args, tool="helgrind")
    expect(not r.timed_out, "H1: helgrind run timed out")
    summary = parse_helgrind_summary(r.stderr)
    _assert_clean_helgrind(summary, "H1")


# ---------------------------------------------------------------------------
# H2 - Helgrind under higher contention (more coders, edf)
# ---------------------------------------------------------------------------
@test("H2 - zero helgrind errors under higher contention (8 coders, edf)", tags=TAGS)
def test_h2_clean_helgrind_contention():
    args = config.build_args(8, 5000, 300, 150, 150, 3, 150, "edf")
    r = run_under_valgrind(args, tool="helgrind", timeout=config.VALGRIND_TIMEOUT)
    expect(not r.timed_out, "H2: helgrind run timed out")
    summary = parse_helgrind_summary(r.stderr)
    _assert_clean_helgrind(summary, "H2")
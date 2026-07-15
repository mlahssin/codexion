"""
parser_tests.py - Argument parsing & validation (QA checklist section 1, P1-P13).

Covers: missing/extra args, non-numeric values, negative numbers, zero values,
overflow (INT_MAX/INT_MIN and beyond), empty strings, whitespace, explicit
plus sign, decimals, and scheduler-name strictness.

The shared invariant checked everywhere here: invalid input must be rejected
immediately (no hang), with a non-zero exit code and *zero* simulation log
lines (no thread should ever have been created).
"""

from framework import test, expect, expect_rejected, run_codexion, parse_log
import config

VALID = config.build_args(4, 5000, 1000, 1000, 1000, 3, 500, "fifo")


# ---------------------------------------------------------------------------
# P1 - Missing arguments
# ---------------------------------------------------------------------------
@test("P1 - missing mandatory argument (scheduler omitted)", tags=["parser"])
def test_p1_missing_argument():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "3", "500"], context="P1")


# ---------------------------------------------------------------------------
# P2 - Extra arguments
# ---------------------------------------------------------------------------
@test("P2 - extra trailing argument", tags=["parser"])
def test_p2_extra_argument():
    expect_rejected(VALID + ["extra_garbage"], context="P2")


# ---------------------------------------------------------------------------
# P3 - Non-numeric value
# ---------------------------------------------------------------------------
@test("P3 - non-numeric value in numeric field", tags=["parser"])
def test_p3_non_numeric():
    expect_rejected(["four", "5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P3")


# ---------------------------------------------------------------------------
# P4 - Negative numbers (each numeric field independently)
# ---------------------------------------------------------------------------
@test("P4 - negative number_of_coders", tags=["parser"])
def test_p4_negative_coders():
    expect_rejected(["-4", "5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P4/coders")


@test("P4 - negative time_to_burnout", tags=["parser"])
def test_p4_negative_burnout():
    expect_rejected(["4", "-5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P4/burnout")


@test("P4 - negative number_of_compiles_required", tags=["parser"])
def test_p4_negative_compiles_required():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "-3", "500", "fifo"], context="P4/compiles")


@test("P4 - negative dongle_cooldown", tags=["parser"])
def test_p4_negative_cooldown():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "3", "-500", "fifo"], context="P4/cooldown")


# ---------------------------------------------------------------------------
# P5 - Zero values (documented convention: coders=0 always rejected;
# number_of_compiles_required=0 must be accepted and stop immediately)
# ---------------------------------------------------------------------------
@test("P5 - number_of_coders = 0 is rejected", tags=["parser"])
def test_p5_zero_coders():
    expect_rejected(["0", "5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P5/coders")


@test("P5 - number_of_compiles_required = 0 stops immediately, no burnout", tags=["parser"])
def test_p5_zero_compiles_required():
    r = run_codexion(["4", "5000", "1000", "1000", "1000", "0", "500", "fifo"],
                      timeout=config.SHORT_TIMEOUT)
    expect(not r.timed_out, "P5/compiles_required=0: simulation should stop immediately")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"P5: malformed log lines: {malformed[:3]}")
    burned = [e for e in events if e.kind == "burned_out"]
    expect(len(burned) == 0,
           "P5/compiles_required=0: condition is already satisfied, no burnout expected")


@test("P5 - dongle_cooldown = 0 is accepted (no wait)", tags=["parser"])
def test_p5_zero_cooldown():
    r = run_codexion(["2", "8000", "300", "100", "100", "2", "0", "fifo"],
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "P5/cooldown=0: run did not complete")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"P5: malformed log lines: {malformed[:3]}")


# ---------------------------------------------------------------------------
# P6 - Overflow / values beyond INT_MAX
# ---------------------------------------------------------------------------
@test("P6 - value beyond INT_MAX is rejected, not wrapped", tags=["parser"])
def test_p6_overflow_int_max():
    expect_rejected(["4", "2147483648", "1000", "1000", "1000", "3", "500", "fifo"],
                     context="P6/INT_MAX+1")


@test("P6 - absurdly long digit string is rejected", tags=["parser"])
def test_p6_overflow_huge_digits():
    expect_rejected(["4", "99999999999999999999", "1000", "1000", "1000", "3", "500", "fifo"],
                     context="P6/huge digits")


# ---------------------------------------------------------------------------
# P7 - INT_MIN
# ---------------------------------------------------------------------------
@test("P7 - INT_MIN is rejected like any negative value", tags=["parser"])
def test_p7_int_min():
    expect_rejected(["4", "-2147483648", "1000", "1000", "1000", "3", "500", "fifo"], context="P7")


# ---------------------------------------------------------------------------
# P8 - Empty string argument
# ---------------------------------------------------------------------------
@test("P8 - empty string argument is rejected", tags=["parser"])
def test_p8_empty_string():
    expect_rejected(["", "5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P8")


# ---------------------------------------------------------------------------
# P9 - Whitespace/tabs inside argument
# ---------------------------------------------------------------------------
@test("P9 - trailing whitespace inside numeric argument is rejected", tags=["parser"])
def test_p9_trailing_whitespace():
    expect_rejected(["4 ", "5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P9/trailing")


@test("P9 - leading tab inside numeric argument is rejected", tags=["parser"])
def test_p9_leading_tab():
    expect_rejected(["\t4", "5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P9/tab")


@test("P9 - leading whitespace in second argument is rejected", tags=["parser"])
def test_p9_leading_space_second_field():
    expect_rejected(["4", " 5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P9/space2")


# ---------------------------------------------------------------------------
# P10 - Explicit plus sign: only check for consistency (no hang/crash), since
# the subject allows either policy as long as it's documented & consistent.
# ---------------------------------------------------------------------------
@test("P10 - leading plus sign handled consistently (no crash)", tags=["parser"])
def test_p10_plus_sign():
    r = run_codexion(["+4", "5000", "1000", "1000", "1000", "3", "500", "fifo"],
                      timeout=config.SHORT_TIMEOUT)
    expect(not r.timed_out, "P10: process hung on '+4'")
    # Either fully rejected (no logs) or fully accepted (valid run) - no
    # crash and no partial/garbled state is acceptable either way.
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"P10: malformed log lines: {malformed[:3]}")
    if r.returncode == 0:
        expect(len(events) > 0, "P10: accepted '+4' but produced no simulation logs")
    else:
        expect(len(events) == 0, "P10: rejected '+4' but still produced simulation logs")


# ---------------------------------------------------------------------------
# P11 - Decimal / non-integer numeric value
# ---------------------------------------------------------------------------
@test("P11 - decimal value in integer field is rejected", tags=["parser"])
def test_p11_decimal_value():
    expect_rejected(["4.5", "5000", "1000", "1000", "1000", "3", "500", "fifo"], context="P11")


# ---------------------------------------------------------------------------
# P12 - Invalid scheduler name (case sensitivity, no partial match)
# ---------------------------------------------------------------------------
@test("P12 - scheduler 'FIFO' (wrong case) is rejected", tags=["parser"])
def test_p12_scheduler_uppercase():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "3", "500", "FIFO"], context="P12/FIFO")


@test("P12 - scheduler 'Fifo' (mixed case) is rejected", tags=["parser"])
def test_p12_scheduler_mixedcase():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "3", "500", "Fifo"], context="P12/Fifo")


@test("P12 - unknown scheduler name is rejected", tags=["parser"])
def test_p12_scheduler_unknown():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "3", "500", "round_robin"],
                     context="P12/round_robin")


@test("P12 - empty scheduler string is rejected", tags=["parser"])
def test_p12_scheduler_empty():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "3", "500", ""], context="P12/empty")


@test("P12 - scheduler with trailing content ('fifo ') is rejected", tags=["parser"])
def test_p12_scheduler_trailing_content():
    expect_rejected(["4", "5000", "1000", "1000", "1000", "3", "500", "fifo "],
                     context="P12/fifo-with-space")


# ---------------------------------------------------------------------------
# P13 - Valid scheduler names (sanity, both must be accepted and complete)
# ---------------------------------------------------------------------------
@test("P13 - scheduler 'fifo' is accepted and completes", tags=["parser"])
def test_p13_fifo_accepted():
    r = run_codexion(["4", "5000", "1000", "1000", "1000", "3", "500", "fifo"],
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "P13/fifo: did not complete in time")
    expect(r.returncode == 0, f"P13/fifo: expected clean exit, got {r.returncode}")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"P13/fifo: malformed lines: {malformed[:3]}")
    expect(len(events) > 0, "P13/fifo: expected simulation logs")


@test("P13 - scheduler 'edf' is accepted and completes", tags=["parser"])
def test_p13_edf_accepted():
    r = run_codexion(["4", "5000", "1000", "1000", "1000", "3", "500", "edf"],
                      timeout=config.DEFAULT_TIMEOUT)
    expect(not r.timed_out, "P13/edf: did not complete in time")
    expect(r.returncode == 0, f"P13/edf: expected clean exit, got {r.returncode}")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0, f"P13/edf: malformed lines: {malformed[:3]}")
    expect(len(events) > 0, "P13/edf: expected simulation logs")
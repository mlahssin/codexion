"""
config.py - Central configuration for the Codexion automated test suite.

Everything a test needs to know about *where the binary lives* and *how
lenient to be about timing* goes here so the rest of the suite never hard-codes
a path or a magic number.
"""

import os

# ---------------------------------------------------------------------------
# Binary location
# ---------------------------------------------------------------------------
# tests/ lives at codexion/tests/, the binary is expected at codexion/codexion
# unless overridden with the CODEXION_BIN environment variable.
_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT = os.path.dirname(_THIS_DIR)

BINARY = os.environ.get("CODEXION_BIN", os.path.join(_PROJECT_ROOT, "codexion"))

# ---------------------------------------------------------------------------
# Timeouts (seconds unless noted)
# ---------------------------------------------------------------------------
SHORT_TIMEOUT = 5          # rejection / invalid-input tests: must exit almost instantly
DEFAULT_TIMEOUT = 15       # normal small simulations
LONG_TIMEOUT = 60          # stress / starvation liveness tests
VALGRIND_TIMEOUT = 90      # valgrind/helgrind runs are much slower

# ---------------------------------------------------------------------------
# Timing tolerances (milliseconds)
# ---------------------------------------------------------------------------
PHASE_TOLERANCE_MS = 25        # compile/debug/refactor duration tolerance (OS jitter)
BURNOUT_TOLERANCE_MS = 10      # subject-mandated hard requirement
COOLDOWN_TOLERANCE_MS = 5      # dongle_cooldown must be respected, allow tiny slack
STARTUP_GRACE_MS = 50          # generous grace period right after process start
STOP_GRACE_MS = 30             # grace period for in-flight logs after stop signal

# ---------------------------------------------------------------------------
# External tools
# ---------------------------------------------------------------------------
VALGRIND_BIN = os.environ.get("VALGRIND_BIN", "valgrind")

# ---------------------------------------------------------------------------
# Log line format, exactly as specified in the subject (Chapter V):
#   timestamp_in_ms X has taken a dongle
#   timestamp_in_ms X is compiling
#   timestamp_in_ms X is debugging
#   timestamp_in_ms X is refactoring
#   timestamp_in_ms X burned out
# ---------------------------------------------------------------------------
LOG_LINE_PATTERN = (
    r"^(?P<ts>\d+) (?P<coder>\d+) "
    r"(?P<event>has taken a dongle|is compiling|is debugging|is refactoring|burned out)$"
)

VALID_SCHEDULERS = ("fifo", "edf")


def build_args(number_of_coders, time_to_burnout, time_to_compile, time_to_debug,
               time_to_refactor, number_of_compiles_required, dongle_cooldown,
               scheduler):
    """Build the argv list in the exact mandatory order defined by the subject."""
    return [
        str(number_of_coders),
        str(time_to_burnout),
        str(time_to_compile),
        str(time_to_debug),
        str(time_to_refactor),
        str(number_of_compiles_required),
        str(dongle_cooldown),
        str(scheduler),
    ]


# A generous, "everything should just work" baseline configuration reused by
# many tests as a starting point (then tweaked per-test).
BASELINE = dict(
    number_of_coders=4,
    time_to_burnout=10000,
    time_to_compile=500,
    time_to_debug=200,
    time_to_refactor=200,
    number_of_compiles_required=3,
    dongle_cooldown=200,
    scheduler="fifo",
)
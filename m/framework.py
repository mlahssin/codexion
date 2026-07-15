"""
framework.py - Shared plumbing used by every *_tests.py module:

  * running the codexion binary (foreground / background / under valgrind)
  * parsing its log output into structured events
  * assertion helpers with readable failure messages
  * a lightweight test registry + runner + colored reporting

Nothing in here is specific to any single QA test - it's the toolbox.
"""

import os
import re
import shutil
import signal
import subprocess
import time
from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional, Tuple

import config


# ===========================================================================
# Exceptions used to drive the runner
# ===========================================================================

class TestFailure(AssertionError):
    """Raised (or bubbled up from expect()) when a test's assertion fails."""


class SkipTest(Exception):
    """Raised when a test cannot meaningfully run (missing binary/tool)."""


# ===========================================================================
# Running the binary
# ===========================================================================

@dataclass
class RunResult:
    returncode: int
    stdout: str
    stderr: str
    duration: float          # wall-clock seconds
    timed_out: bool
    signal: Optional[int] = None


def binary_available() -> bool:
    return os.path.isfile(config.BINARY) and os.access(config.BINARY, os.X_OK)


def require_binary():
    if not binary_available():
        raise SkipTest(f"codexion binary not found/executable at: {config.BINARY}")


def require_tool(name: str):
    if shutil.which(name) is None:
        raise SkipTest(f"required external tool '{name}' not found on PATH")


def run_codexion(cmd_args: List[str], timeout: float = config.DEFAULT_TIMEOUT) -> RunResult:
    """Run codexion to completion (or until timeout) and capture output."""
    require_binary()
    start = time.monotonic()
    try:
        proc = subprocess.run(
            [config.BINARY, *cmd_args],
            capture_output=True, text=True, timeout=timeout,
        )
        duration = time.monotonic() - start
        sig = -proc.returncode if proc.returncode < 0 else None
        return RunResult(proc.returncode, proc.stdout, proc.stderr, duration, False, sig)
    except subprocess.TimeoutExpired as e:
        duration = time.monotonic() - start
        stdout = e.stdout or ""
        stderr = e.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode(errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode(errors="replace")
        return RunResult(-1, stdout, stderr, duration, True, None)


def run_codexion_bg(cmd_args: List[str]) -> subprocess.Popen:
    """Start codexion in the background; caller is responsible for cleanup."""
    require_binary()
    return subprocess.Popen(
        [config.BINARY, *cmd_args],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )


def kill_quietly(proc: subprocess.Popen):
    if proc.poll() is None:
        try:
            proc.send_signal(signal.SIGKILL)
        except Exception:
            pass
        try:
            proc.wait(timeout=5)
        except Exception:
            pass


def thread_count(pid: int) -> Optional[int]:
    """Read live thread count for a running pid via /proc (Linux only)."""
    status_path = f"/proc/{pid}/status"
    try:
        with open(status_path) as f:
            for line in f:
                if line.startswith("Threads:"):
                    return int(line.split()[1])
    except FileNotFoundError:
        return None
    return None


def process_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


# ===========================================================================
# Log parsing
# ===========================================================================

@dataclass(frozen=True)
class Event:
    ts: int
    coder: int
    kind: str   # one of: taken, compiling, debugging, refactoring, burned_out


_KIND_MAP = {
    "has taken a dongle": "taken",
    "is compiling": "compiling",
    "is debugging": "debugging",
    "is refactoring": "refactoring",
    "burned out": "burned_out",
}

_LINE_RE = re.compile(config.LOG_LINE_PATTERN)


def parse_log(stdout: str) -> Tuple[List[Event], List[str]]:
    """
    Parse raw stdout into a list of well-formed Events, plus a list of any
    lines that did NOT match the required log format (interleaved / corrupted
    / stray debug output).
    """
    events: List[Event] = []
    malformed: List[str] = []
    for line in stdout.splitlines():
        if line.strip() == "":
            continue
        m = _LINE_RE.match(line)
        if m:
            events.append(Event(int(m.group("ts")), int(m.group("coder")),
                                 _KIND_MAP[m.group("event")]))
        else:
            malformed.append(line)
    return events, malformed


def group_by_coder(events: List[Event]) -> Dict[int, List[Event]]:
    out: Dict[int, List[Event]] = {}
    for e in events:
        out.setdefault(e.coder, []).append(e)
    return out


@dataclass
class Cycle:
    """One compile/debug/refactor cycle reconstructed from the log."""
    dongle_ts: List[int]
    compile_ts: Optional[int] = None
    debug_ts: Optional[int] = None
    refactor_ts: Optional[int] = None


def compile_cycles(coder_events: List[Event]) -> List[Cycle]:
    """
    Reconstruct compile cycles for a single coder's event stream, following
    the subject's mandated order:
        taken, taken, compiling, debugging, refactoring, (repeat)
    A trailing incomplete cycle (e.g. interrupted by simulation stop) is
    still included with whatever fields were observed.
    """
    cycles: List[Cycle] = []
    pending_dongles: List[int] = []
    cur: Optional[Cycle] = None

    for ev in coder_events:
        if ev.kind == "taken":
            pending_dongles.append(ev.ts)
        elif ev.kind == "compiling":
            cur = Cycle(dongle_ts=pending_dongles, compile_ts=ev.ts)
            pending_dongles = []
            cycles.append(cur)
        elif ev.kind == "debugging":
            if cur is not None:
                cur.debug_ts = ev.ts
        elif ev.kind == "refactoring":
            if cur is not None:
                cur.refactor_ts = ev.ts
        elif ev.kind == "burned_out":
            pass  # handled separately by callers interested in burnout
    return cycles


def burnout_event(events: List[Event]) -> Optional[Event]:
    for e in events:
        if e.kind == "burned_out":
            return e
    return None


def last_timestamp(events: List[Event]) -> int:
    return max((e.ts for e in events), default=0)


def find_overlaps(intervals: List[Tuple[int, int, str]]) -> List[Tuple[Tuple, Tuple]]:
    """
    intervals: list of (start, end, label). Returns pairs of intervals that
    overlap in time (end is exclusive-ish; touching endpoints are OK).
    """
    overlaps = []
    ivs = sorted(intervals, key=lambda t: t[0])
    for i in range(len(ivs)):
        for j in range(i + 1, len(ivs)):
            a, b = ivs[i], ivs[j]
            if b[0] < a[1]:
                overlaps.append((a, b))
    return overlaps


# ===========================================================================
# Valgrind / Helgrind helpers
# ===========================================================================

def run_under_valgrind(cmd_args: List[str], tool: str = "memcheck",
                        timeout: float = config.VALGRIND_TIMEOUT,
                        extra_args: Optional[List[str]] = None) -> RunResult:
    require_tool(config.VALGRIND_BIN)
    require_binary()
    vg_cmd = [config.VALGRIND_BIN, f"--tool={tool}"]
    if tool == "memcheck":
        vg_cmd += ["--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes"]
    if extra_args:
        vg_cmd += extra_args
    vg_cmd += [config.BINARY, *cmd_args]

    start = time.monotonic()
    try:
        proc = subprocess.run(vg_cmd, capture_output=True, text=True, timeout=timeout)
        duration = time.monotonic() - start
        return RunResult(proc.returncode, proc.stdout, proc.stderr, duration, False)
    except subprocess.TimeoutExpired as e:
        duration = time.monotonic() - start
        stdout = e.stdout or ""
        stderr = e.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode(errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode(errors="replace")
        return RunResult(-1, stdout, stderr, duration, True)


def parse_memcheck_summary(vg_stderr: str) -> Dict[str, int]:
    def grab(pattern, default=0):
        m = re.search(pattern, vg_stderr)
        return int(m.group(1).replace(",", "")) if m else default

    return {
        "definitely_lost": grab(r"definitely lost:\s*([\d,]+) bytes"),
        "indirectly_lost": grab(r"indirectly lost:\s*([\d,]+) bytes"),
        "possibly_lost": grab(r"possibly lost:\s*([\d,]+) bytes"),
        "still_reachable": grab(r"still reachable:\s*([\d,]+) bytes"),
        "errors": grab(r"ERROR SUMMARY:\s*(\d+) errors"),
        "invalid_rw": len(re.findall(r"Invalid (read|write)", vg_stderr)),
    }


def parse_helgrind_summary(vg_stderr: str) -> Dict[str, int]:
    def grab(pattern, default=0):
        m = re.search(pattern, vg_stderr)
        return int(m.group(1)) if m else default

    return {
        "errors": grab(r"ERROR SUMMARY:\s*(\d+) errors"),
        "data_races": len(re.findall(r"Possible data race", vg_stderr)),
        "lock_order": len(re.findall(r"lock order", vg_stderr, re.IGNORECASE)),
        "unlocked_not_locked": len(re.findall(r"unlocked a not-locked lock", vg_stderr)),
    }


# ===========================================================================
# Assertion helpers
# ===========================================================================

def expect(cond: bool, msg: str):
    if not cond:
        raise TestFailure(msg)


def expect_approx(actual: float, expected: float, tol: float, msg: str = ""):
    if abs(actual - expected) > tol:
        raise TestFailure(
            f"{msg} (expected {expected} \u00b1{tol}, got {actual}, "
            f"delta={abs(actual - expected)})".strip()
        )


def expect_equal(actual, expected, msg: str = ""):
    if actual != expected:
        raise TestFailure(f"{msg} (expected {expected!r}, got {actual!r})".strip())


def expect_rejected(cmd_args: List[str], timeout: float = config.SHORT_TIMEOUT,
                     context: str = ""):
    """
    Common pattern for parser/boundary tests: invalid input must be rejected
    quickly, cleanly, with no simulation logs and a non-zero exit code.
    """
    r = run_codexion(cmd_args, timeout=timeout)
    expect(not r.timed_out,
           f"{context}: process hung instead of rejecting invalid input {cmd_args}")
    expect(r.returncode != 0,
           f"{context}: expected non-zero exit code for invalid input {cmd_args}, "
           f"got {r.returncode}")
    events, _ = parse_log(r.stdout)
    expect(len(events) == 0,
           f"{context}: simulation produced state logs on invalid input {cmd_args}: "
           f"{events[:3]}")
    return r


def expect_accepted_and_completes(cmd_args: List[str], timeout: float = config.DEFAULT_TIMEOUT,
                                   context: str = "") -> Tuple[RunResult, List[Event], List[str]]:
    r = run_codexion(cmd_args, timeout=timeout)
    expect(not r.timed_out, f"{context}: run did not complete within {timeout}s: {cmd_args}")
    events, malformed = parse_log(r.stdout)
    expect(len(malformed) == 0,
           f"{context}: malformed/corrupted log lines found: {malformed[:3]}")
    return r, events, malformed


# ===========================================================================
# Test registry + runner
# ===========================================================================

@dataclass
class _RegisteredTest:
    name: str
    func: Callable
    tags: set
    module: str


_REGISTRY: List[_RegisteredTest] = []


def test(name: str, tags: Optional[List[str]] = None):
    """Decorator used by every *_tests.py file to register a test case."""
    def deco(fn: Callable):
        _REGISTRY.append(_RegisteredTest(
            name=name, func=fn, tags=set(tags or []), module=fn.__module__,
        ))
        return fn
    return deco


def get_registry() -> List[_RegisteredTest]:
    return list(_REGISTRY)


class Color:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    CYAN = "\033[96m"
    BOLD = "\033[1m"
    END = "\033[0m"

    @staticmethod
    def wrap(code, text):
        return f"{code}{text}{Color.END}"


@dataclass
class TestOutcome:
    name: str
    module: str
    status: str   # PASS / FAIL / ERROR / SKIP
    message: str
    duration: float


def run_tests(tests: List[_RegisteredTest], verbose: bool = True) -> List[TestOutcome]:
    outcomes: List[TestOutcome] = []
    for t in tests:
        start = time.monotonic()
        try:
            t.func()
            status, message = "PASS", ""
        except SkipTest as s:
            status, message = "SKIP", str(s)
        except TestFailure as f:
            status, message = "FAIL", str(f)
        except Exception as e:  # unexpected crash bug in the *test itself*
            status, message = "ERROR", f"{type(e).__name__}: {e}"
        duration = time.monotonic() - start
        outcome = TestOutcome(t.name, t.module, status, message, duration)
        outcomes.append(outcome)
        if verbose:
            print(_format_outcome(outcome))
    return outcomes


def _format_outcome(o: TestOutcome) -> str:
    color = {
        "PASS": Color.GREEN, "FAIL": Color.RED,
        "ERROR": Color.RED, "SKIP": Color.YELLOW,
    }[o.status]
    line = f"  {Color.wrap(color, f'[{o.status:^5}]')} {o.name}  ({o.duration:.2f}s)"
    if o.message:
        line += f"\n           {Color.wrap(Color.CYAN, '->')} {o.message}"
    return line


def print_summary(outcomes: List[TestOutcome]):
    total = len(outcomes)
    by_status = {}
    for o in outcomes:
        by_status.setdefault(o.status, []).append(o)

    print()
    print(Color.wrap(Color.BOLD, "=" * 60))
    print(Color.wrap(Color.BOLD, "SUMMARY"))
    print(Color.wrap(Color.BOLD, "=" * 60))
    for status in ("PASS", "FAIL", "ERROR", "SKIP"):
        n = len(by_status.get(status, []))
        color = {"PASS": Color.GREEN, "FAIL": Color.RED,
                 "ERROR": Color.RED, "SKIP": Color.YELLOW}[status]
        print(f"  {Color.wrap(color, status):<20} {n}/{total}")

    failed = by_status.get("FAIL", []) + by_status.get("ERROR", [])
    if failed:
        print()
        print(Color.wrap(Color.RED, "Failed tests:"))
        for o in failed:
            print(f"  - {o.name}: {o.message}")
    print()
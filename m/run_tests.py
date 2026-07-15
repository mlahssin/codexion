#!/usr/bin/env python3
"""
run_tests.py - Entry point for the Codexion automated test suite.

Usage:
    python3 run_tests.py                     # run everything except slow/valgrind/helgrind
    python3 run_tests.py --all               # run absolutely everything (slow)
    python3 run_tests.py --with-valgrind      # also run memcheck + helgrind suites
    python3 run_tests.py --tags fifo edf      # only run tests tagged 'fifo' or 'edf'
    python3 run_tests.py --exclude-tags slow  # run everything except tests tagged 'slow'
    python3 run_tests.py --list               # list every registered test and exit
    python3 run_tests.py --binary ../codexion # override the binary path for this run

Test categories map 1:1 onto the QA checklist that shipped with the subject:
    parser      -> P1-P13   (argument validation)
    boundary    -> B1-B6, F2, E1, E2, E5
    fifo        -> SCH1, SCH4(fifo), F4
    edf         -> SCH2, SCH3, SCH4(edf)
    timing      -> F1, T1, T2, T4, T5
    cooldown    -> T3, F3
    starvation  -> ST1-ST3
    deadlock    -> D1-D2, SYN1, SYN2, SYN4
    valgrind    -> V1-V3           (slow, opt-in)
    helgrind    -> H1-H2           (slow, opt-in)
"""

import argparse
import os
import sys

import config
import framework

# Importing each *_tests module has the side effect of registering its tests
# (via the @framework.test decorator) into framework's global registry.
import parser_tests      # noqa: F401
import boundary_tests    # noqa: F401
import fifo_tests        # noqa: F401
import edf_tests         # noqa: F401
import timing_tests      # noqa: F401
import cooldown_tests    # noqa: F401
import starvation_tests  # noqa: F401
import deadlock_tests    # noqa: F401
import valgrind_tests    # noqa: F401
import helgrind_tests    # noqa: F401


DEFAULT_EXCLUDED_TAGS = {"valgrind", "helgrind"}


def parse_cli():
    p = argparse.ArgumentParser(description="Codexion automated test suite")
    p.add_argument("--binary", help="path to the codexion binary "
                                     "(overrides CODEXION_BIN / default)")
    p.add_argument("--all", action="store_true",
                    help="run every test, including valgrind/helgrind (slow)")
    p.add_argument("--with-valgrind", action="store_true",
                    help="also include valgrind/helgrind tests")
    p.add_argument("--tags", nargs="*", default=None,
                    help="only run tests that have at least one of these tags")
    p.add_argument("--exclude-tags", nargs="*", default=None,
                    help="skip tests that have any of these tags")
    p.add_argument("--module", nargs="*", default=None,
                    help="only run tests from these modules (e.g. parser_tests)")
    p.add_argument("--list", action="store_true",
                    help="list all registered tests and exit (no execution)")
    return p.parse_args()


def select_tests(all_tests, args):
    selected = list(all_tests)

    if args.module:
        wanted = set(args.module)
        selected = [t for t in selected if t.module in wanted]

    if args.tags:
        wanted = set(args.tags)
        selected = [t for t in selected if t.tags & wanted]

    exclude = set(args.exclude_tags or [])
    if not (args.all or args.with_valgrind or args.tags):
        # By default, keep the suite fast: skip valgrind/helgrind unless
        # explicitly requested via --all, --with-valgrind, or --tags.
        exclude |= DEFAULT_EXCLUDED_TAGS

    if exclude:
        selected = [t for t in selected if not (t.tags & exclude)]

    return selected


def main():
    args = parse_cli()

    if args.binary:
        config.BINARY = os.path.abspath(args.binary)

    all_tests = framework.get_registry()

    if args.list:
        for t in all_tests:
            tags = ",".join(sorted(t.tags)) or "-"
            print(f"[{t.module:<18}] {t.name}  (tags: {tags})")
        print(f"\n{len(all_tests)} tests registered.")
        return 0

    selected = select_tests(all_tests, args)

    print(framework.Color.wrap(framework.Color.BOLD,
          f"Codexion test suite - binary: {config.BINARY}"))
    if not framework.binary_available():
        print(framework.Color.wrap(
            framework.Color.YELLOW,
            f"WARNING: binary not found or not executable at {config.BINARY} "
            f"- most tests will report SKIP.\n"
            f"Build it first, or point --binary at the compiled executable."
        ))
    print(f"Running {len(selected)}/{len(all_tests)} tests "
          f"(use --list to see all, --all to include valgrind/helgrind)\n")

    outcomes = framework.run_tests(selected, verbose=True)
    framework.print_summary(outcomes)

    failed = any(o.status in ("FAIL", "ERROR") for o in outcomes)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
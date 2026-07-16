*This project has been created as part of the 42 curriculum by mlahssin.*

# Codexion

Master the race for resources before the deadline masters you.

## Table of contents

- [Description](#description)
- [Instructions](#instructions)
- [Usage](#usage)
- [Project structure](#project-structure)
- [Blocking cases handled](#blocking-cases-handled)
- [Thread synchronization mechanisms](#thread-synchronization-mechanisms)
- [Resources](#resources)

## Description

Codexion is a C simulation of a classic resource-sharing/synchronization problem
(a themed variant of the Dining Philosophers problem), built with POSIX threads.

A number of **coders** sit in a circular co-working hub around a shared quantum
compiler. Each coder repeatedly cycles through three phases: **compile**,
**debug**, and **refactor**. Compiling is the only phase that requires a
resource: it needs **two USB dongles** held simultaneously, one shared with
the coder's left neighbor and one shared with the coder's right neighbor.
There are exactly as many dongles as coders, arranged in a ring, so every
dongle is contested by exactly two neighboring coders.

If a coder does not manage to start compiling again within `time_to_burnout`
milliseconds of their last compile (or of the simulation's start), they
**burn out** and the simulation stops. The simulation can also stop
successfully once every coder has compiled at least `number_of_compiles_required`
times.

The goal of the project is to orchestrate all of this correctly and safely
with threads, mutexes, and condition variables: no coder should ever be able
to duplicate a dongle, no two coders should be able to deadlock each other
over a shared dongle, and no coder should starve of dongles indefinitely
under a fair scheduling policy.

## Instructions

### Compilation

```sh
make        # builds the codexion binary
make clean  # removes object files
make fclean # removes object files and the binary
make re     # fclean + all
```

The Makefile compiles every source file with `-Wall -Wextra -Werror` using `cc`.

### Execution

```sh
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

Example:

```sh
./codexion 5 800 200 100 100 5 50 edf
```

If any argument is missing, non-numeric, negative, or if `scheduler` is
anything other than `fifo` or `edf`, the program rejects the input and exits
with an error instead of running.

## Usage

| Argument                     | Meaning                                                                                          |
|-------------------------------|---------------------------------------------------------------------------------------------------|
| `number_of_coders`            | Number of coders (and number of dongles) in the simulation.                                       |
| `time_to_burnout`              | Milliseconds a coder may go without starting a compile before burning out.                        |
| `time_to_compile`              | Milliseconds a compile phase takes (requires holding both dongles).                               |
| `time_to_debug`                | Milliseconds a debug phase takes.                                                                 |
| `time_to_refactor`             | Milliseconds a refactor phase takes.                                                              |
| `number_of_compiles_required`  | Simulation stops successfully once every coder has reached this many compiles.                    |
| `dongle_cooldown`              | Milliseconds a dongle stays unavailable after being released, before it can be taken again.       |
| `scheduler`                    | Arbitration policy for contested dongles: `fifo` (first request served first) or `edf` (earliest burnout deadline served first). |

Every state change of a coder is logged as `timestamp_in_ms coder_id message`,
for example:

```
0 1 has taken a dongle
1 1 has taken a dongle
1 1 is compiling
201 1 is debugging
401 1 is refactoring
```

## Project structure

```
.
├── Makefile
├── include/
│   └── codexion.h       # shared types (t_coder, t_dongle, t_shared, t_params) and prototypes
└── src1/
    ├── main.c           # program entry point, coder/monitor routines, dongle acquire/release logic
    ├── parsing.c         # argument parsing and validation
    ├── params_init.c     # coder/dongle/shared-state initialization
    └── heap.c            # fixed-capacity binary min-heap used for dongle scheduling
```

## Blocking cases handled

- **Deadlock avoidance (Coffman's conditions).** Mutual exclusion and
  hold-and-wait are inherent to the problem (a dongle can only be held by one
  coder at a time, and a coder must hold its first dongle while waiting for
  its second). The classic failure mode this creates is *circular wait*:
  every coder grabbing their left dongle first and then waiting forever on
  their right one. This is broken by alternating acquisition order between
  coders: even-indexed coders acquire their lower-indexed dongle first, then
  the higher one, while odd-indexed coders acquire in the opposite order. This
  guarantees that at least one link in any potential cycle is reversed, so a
  full circular hold-and-wait chain can never form.
- **Starvation prevention.** Each dongle is contested by exactly two
  neighboring coders, so its waiting list never needs to hold more than two
  entries. Access is arbitrated by a small binary min-heap ordered either by
  arrival time (`fifo`) or by earliest burnout deadline (`edf`), with a
  deterministic tie-break on coder id so two equal priorities never produce
  ambiguous ordering. This guarantees a waiting coder is always eventually
  served rather than being repeatedly passed over.
- **Cooldown handling.** Every dongle records the timestamp at which it was
  released plus `dongle_cooldown`. A coder that is otherwise next in line but
  still inside the cooldown window is parked with `pthread_cond_timedwait`
  until exactly that deadline, instead of busy-polling the lock.
- **Burnout detection.** A dedicated monitor thread, separate from every
  coder thread, continuously compares each coder's `last_compile_start +
  time_to_burnout` against the current simulation time and immediately
  raises a global stop condition as soon as a coder crosses its deadline,
  independently of what any coder thread is doing at that moment.
- **Log serialization.** Every state-change message is emitted through a
  single logging function that holds one global print mutex for the
  duration of the print, so two messages from different threads can never
  interleave into a single garbled line.

## Thread synchronization mechanisms

- **`pthread_mutex_t print_mutex`** — a single global mutex guarding all
  `printf` calls that report coder state changes, guaranteeing atomic,
  non-interleaved log lines across all coder threads and the monitor thread.
- **`pthread_mutex_t stop_mutex`** — protects the shared `stop` flag. It is
  never read or written directly; all access goes through `get_stop()` /
  `set_stop()` helpers, so every thread (coders and monitor alike) observes a
  consistent view of whether the simulation should end.
- **`pthread_mutex_t compile_count_mutex`** (one per coder) — protects that
  coder's `compile_count` and `last_compile_start` fields. These are written
  by the coder's own thread when it starts or finishes a compile, and read
  concurrently by the monitor thread when checking for burnout or for the
  "all coders finished" stopping condition — the mutex is what makes that
  cross-thread read/write safe.
- **`pthread_mutex_t dongle_mutex` + `pthread_cond_t dongle_wait`** (one pair
  per dongle) — together they protect a dongle's availability state
  (`used`, `released_at`) and its small waiter heap. A coder registers itself
  on the dongle's heap and checks eligibility *under the same mutex*, which
  prevents a race where two coders could both observe "dongle free" and both
  try to take it: only the mutex holder can transition the dongle's state,
  and every other coder blocks on `dongle_wait` until it is signaled.
- **`pthread_cond_wait` / `pthread_cond_timedwait`** — a coder that cannot
  yet take a dongle blocks on the dongle's condition variable rather than
  spinning: `pthread_cond_wait` when it is simply not its turn yet, and
  `pthread_cond_timedwait` with a deadline equal to the cooldown expiry when
  it is next in line but the dongle is still cooling down. Either way, the
  mutex is released while parked and reacquired automatically before the
  coder re-evaluates its condition, so it never holds a lock while idle.
- **Coder ↔ monitor communication.** The monitor thread never touches dongle
  state directly; it only reads coder progress through
  `compile_count_mutex`-protected fields, and communicates back to every
  coder by flipping the shared `stop` flag (under `stop_mutex`) and waking
  any coder currently parked on a dongle's condition variable, so blocked
  threads re-check the stop condition and unwind cleanly instead of waiting
  forever.

## Resources

- POSIX Threads Programming — https://hpc-tutorials.llnl.gov/posix/
- Linux man pages: `pthread_create(3)`, `pthread_mutex_lock(3)`,
  `pthread_cond_wait(3)`, `pthread_cond_timedwait(3)`, `gettimeofday(2)` —
  https://man7.org/linux/man-pages/
- Dijkstra's Dining Philosophers problem —
  https://en.wikipedia.org/wiki/Dining_philosophers_problem
- Coffman's conditions for deadlock —
  https://en.wikipedia.org/wiki/Deadlock#Necessary_conditions
- Earliest Deadline First scheduling —
  https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling
- Binary heap / priority queue —
  *Introduction to Algorithms* (Cormen, Leiserson, Rivest, Stein), chapter on
  heaps and priority queues.

**AI usage:** An AI assistant (Claude) was used as a learning aid to help
understand core threading concepts used in this project — POSIX mutexes,
condition variables, `pthread_cond_wait` vs `pthread_cond_timedwait`, and how
they combine to avoid busy-waiting and races. It was not used to generate the
project's source code; all implementation choices and code were written and
are understood by the author.

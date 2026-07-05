# 🧵 Threads & Mutex — Complete Reference
> Codexion Study Sheet · Built from your mentoring sessions

---

## TABLE OF CONTENTS

1. [What is a Process vs a Thread](#1-process-vs-thread)
2. [Why Threads Exist — The Problem They Solve](#2-why-threads-exist)
3. [POSIX Thread Essentials](#3-posix-thread-essentials)
4. [Passing Data to Threads](#4-passing-data-to-threads)
5. [Race Conditions — The Core Danger](#5-race-conditions)
6. [Mutex — The Solution](#6-mutex)
7. [Deadlock](#7-deadlock)
8. [Parallel Sum — Splitting Work Across Threads](#8-parallel-sum--splitting-work-across-threads)
9. [lock vs trylock](#9-lock-vs-trylock)
10. [Condition Variables](#10-condition-variables)
11. [Producer Consumer Pattern](#11-producer-consumer-pattern)
12. [pthread_exit](#12-pthread_exit)
13. [Barriers](#13-barriers)
14. [Codexion Context](#14-codexion-context)
15. [Cheatsheet](#15-cheatsheet)
16. [Self-Test Questions](#16-self-test-questions)

---

## 1. Process vs Thread

| | **Process** | **Thread** |
|---|---|---|
| Definition | Your entire running program | A worker *inside* a process |
| Memory | Own isolated memory space | Shares memory with sibling threads |
| Creation | OS creates it when you run your program | You create it with `pthread_create` |
| Cost | Heavy (full memory copy) | Light (shares existing memory) |
| Communication | Needs IPC (pipes, sockets...) | Direct — same variables |

```
Process
┌────────────────────────────────────┐
│  Stack   Heap   Code   Data        │
│                                    │
│  Thread 1 ──┐                      │
│  Thread 2 ──┼── all share the heap │
│  Thread 3 ──┘                      │
└────────────────────────────────────┘
```

**Key point:** Each thread has its **own stack** (local variables), but all threads share the **heap** (globals, malloc'd memory).

---

## 2. Why Threads Exist

### The Problem — Sequential Code Can't Simulate Concurrency

Without threads, your program does **one thing at a time**:

```c
coder_1_compiles();   // wait 200ms
coder_2_compiles();   // wait 200ms
coder_3_compiles();   // wait 200ms
coder_4_compiles();   // wait 200ms
// Total: 800ms — WRONG, coders work simultaneously in real life
```

### The Solution — Threads Run in Parallel

```c
pthread_create(&t1, NULL, coder_routine, &coder1);
pthread_create(&t2, NULL, coder_routine, &coder2);
pthread_create(&t3, NULL, coder_routine, &coder3);
pthread_create(&t4, NULL, coder_routine, &coder4);
// All 4 run simultaneously → ~200ms total ✅
```

### Three Reasons to Use Threads

| Reason | Example in Codexion |
|--------|-------------------|
| **True parallelism** | 4 coders compile at the same time |
| **Independent waiting** | One coder waiting for dongle doesn't block others |
| **Background monitoring** | Monitor thread watches for burnout while coders run |

---

## 3. POSIX Thread Essentials

### Creating a Thread

```c
pthread_t thread;

pthread_create(&thread, NULL, function, argument);
//             ^         ^     ^          ^
//             thread ID attrs  function  void* arg
```

### Waiting for a Thread to Finish

```c
pthread_join(thread, NULL);
// Blocks until the thread returns
// MUST be called or you leak resources
```

> ⚠️ **Always join every thread you create before `free()`ing their data.**

### Thread Function Signature — Always This Shape

```c
void *coder_routine(void *arg)
{
    // cast arg back to your type
    t_coder *coder = (t_coder *)arg;

    // do work...

    return (NULL);
}
```

### Function Name vs &Function Name

```c
pthread_create(&t, NULL, coder_routine, arg);   // ✅ correct
pthread_create(&t, NULL, &coder_routine, arg);  // ✅ also works — identical
```

In C, `coder_routine` **is already the address** of the function. Writing `&` is redundant but not wrong.

---

## 4. Passing Data to Threads

### The Bug — Passing a Changing Variable

```c
int i = 0;
while (i < 4)
{
    pthread_create(&threads[i], NULL, coder_routine, &i);  // ❌ BUG
    i++;
}
```

**Why it's broken:** By the time the thread starts, `i` may already have changed. All threads might get the same index.

### The Fix — Pass a Stable Pointer

```c
while (i < 4)
{
    pthread_create(&threads[i], NULL, coder_routine, &sim->coders[i]);  // ✅
    i++;
}
```

Each coder has its own struct — that pointer is stable and unique.

---

## 5. Race Conditions

### What Is a Race Condition?

> When **two threads access shared data at the same time** and the result depends on **who gets there first** — which you cannot control.

### Why `counter++` Is Dangerous

It looks like one operation. It is actually **three CPU steps**:

```
1. READ   → load counter value from memory
2. ADD    → add 1
3. WRITE  → store result back to memory
```

The OS can switch threads **between any of these steps**.

### The Classic Example

```
counter = 5

Thread 1: READ  → gets 5
                    ← OS switches to Thread 2
Thread 2: READ  → gets 5   (not 6 — T1 hasn't written yet)
Thread 2: ADD   → 6
Thread 2: WRITE → counter = 6
Thread 1: ADD   → 6
Thread 1: WRITE → counter = 6   ← overwrites T2's result

Expected: 7     Actual: 6     One increment LOST 💀
```

### Why Race Conditions Are Hard to Find

- **Non-deterministic** — happens only sometimes
- **Timing-dependent** — depends on OS scheduling
- **Disappear with `printf`** — adding print statements changes thread timing, which can mask the bug
- **Worse under load** — more threads = more collisions

### Common Shared Data at Risk in Codexion

| Shared Resource | Risk |
|----------------|------|
| Dongle availability flag | Two coders grab same dongle |
| Compile counter | Lost increments |
| `sim->stopped` flag | Inconsistent stop signal |
| Log output | Garbled prints |

---

## 6. Mutex

### What Is a Mutex?

**Mutual Exclusion lock** — only **one thread** can hold it at a time. Others wait.

```
Thread 1: LOCK ──► [critical section] ──► UNLOCK
Thread 2:          (waiting...)                   ──► LOCK ──► [critical section] ──► UNLOCK
```

### The Pattern — Always This Shape

```c
pthread_mutex_lock(&mutex);
    // critical section — only one thread here at a time
    counter++;
pthread_mutex_unlock(&mutex);
```

### Full Lifecycle

```c
pthread_mutex_t mutex;

// Init (once, before threads start)
pthread_mutex_init(&mutex, NULL);

// Use (in threads)
pthread_mutex_lock(&mutex);
    shared_data++;   // safe
pthread_mutex_unlock(&mutex);

// Destroy (after all threads finish)
pthread_mutex_destroy(&mutex);
```

### What Mutex Makes Atomic

```
Without mutex:     READ ... ADD ... WRITE  ← can be interrupted anywhere
With mutex:        [READ + ADD + WRITE]    ← treated as one uninterruptible unit
```

### Rules

| Rule | Why |
|------|-----|
| Always unlock after lock | Forgetting = all other threads freeze forever |
| Keep critical sections short | Long locks = poor performance |
| Never lock inside a lock (same mutex) | Deadlock |
| Init before use, destroy after | Memory/resource hygiene |

---

## 7. Deadlock

### What Is a Deadlock?

> Two (or more) threads each holding a resource and waiting for one the other holds. **Both wait forever. Nothing moves.**

```
Thread A: holds Mutex 1, waits for Mutex 2 ──┐
Thread B: holds Mutex 2, waits for Mutex 1 ◄─┘
             → Neither can proceed. Frozen.
```

### Coffman's 4 Conditions (all 4 must be true for deadlock)

| # | Condition | Meaning |
|---|-----------|---------|
| 1 | **Mutual exclusion** | Only one thread holds a resource at a time |
| 2 | **Hold and wait** | Thread holds one resource while waiting for another |
| 3 | **No preemption** | Can't forcibly take a resource from another thread |
| 4 | **Circular wait** | A waits for B, B waits for A |

> **Break any ONE condition → deadlock impossible.**

### The Prevention Trick — Always Lock in the Same Order

```c
// ❌ DEADLOCK RISK
// Thread 1: lock(A) then lock(B)
// Thread 2: lock(B) then lock(A)

// ✅ SAFE — both always lock A before B
pthread_mutex_lock(&mutex_a);
pthread_mutex_lock(&mutex_b);
    // critical work
pthread_mutex_unlock(&mutex_b);
pthread_mutex_unlock(&mutex_a);
```

This breaks **condition 4 (circular wait)** — you can never have a cycle.

---

## 8. Parallel Sum — Splitting Work Across Threads

### The Concept
Instead of one thread summing all elements, split the array across threads:

```
Thread 1: data[0] + data[1] + ... + data[4]   (first half)
Thread 2: data[5] + data[6] + ... + data[9]   (second half)
Main:     partial_sum_1 + partial_sum_2        (combine)
```

### Implementation Pattern

```c
int t[10] = {1, 9, 5, 3, 2, 4, 6, 11, 20, 90};

void *suming(void *arg)
{
    int *elt = (int *)arg;   // *elt = starting index
    int sum = 0;
    for (int i = 0; i < 5; i++)
        sum += t[*elt + i];
    *elt = sum;              // reuse the same memory to return the result
    return elt;
}

int main()
{
    pthread_t th[2];
    void *ret;
    int global_sum = 0;

    for (int i = 0; i < 2; i++) {
        int *index = malloc(sizeof(int));   // separate malloc per thread
        *index = i * 5;                     // thread 0 → index 0, thread 1 → index 5
        pthread_create(&th[i], NULL, suming, index);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(th[i], &ret);          // &ret not ret — pthread_join needs void**
        global_sum += *(int *)ret;
        free(ret);                          // free each thread's malloc right after joining
    }

    printf("%d\n", global_sum);
}
```

### Key Lessons from This Example

| Mistake | Why It's Wrong | Fix |
|---|---|---|
| One shared `malloc` for all threads | Same memory overwritten before thread reads it | `malloc` inside the loop |
| `pthread_join(th[i], ret)` | `ret` is `void*`, join needs `void**` | `pthread_join(th[i], &ret)` |
| `free(ret)` once after loop | Only frees last thread's memory — leak | `free(ret)` inside join loop |
| Join inside creation loop | Blocks after each thread — kills parallelism | Two separate loops |

### When Parallelism Actually Pays Off

| Array size | 2 threads better? |
|---|---|
| 10 elements | ❌ thread overhead dominates |
| 10,000 elements | ✓ maybe |
| 10,000,000 elements | ✓ clearly yes |

> **Granularity rule:** Parallelism pays off when the work per thread is large enough to outweigh the cost of creating and syncing threads.

---

## 9. lock vs trylock

### `pthread_mutex_lock` — Blocking
```c
pthread_mutex_lock(&mutex);
// if mutex is taken → thread SLEEPS here until it's free
// OS puts it in a waiting queue, wakes it up automatically
```
- Thread does nothing until it gets the lock
- **Does NOT waste CPU** — OS puts thread to sleep properly

### `pthread_mutex_trylock` — Non-Blocking
```c
if (pthread_mutex_trylock(&mutex) == 0) {
    // got the lock — do work
} else {
    // mutex was busy — do something else
}
```
- Returns immediately whether it gets the lock or not
- Returns `0` on success, non-zero if already taken

### Mental Model
```
lock     → "I need this, wake me up when it's free" 😴
trylock  → "Let me try — if busy I'll go do something else" 🔄
```

### When to Use Each

| Situation | Use |
|---|---|
| You **must** access that resource, no alternative | `lock` |
| You have useful work to do while waiting | `trylock` |
| Simple critical section | `lock` |
| Avoiding deadlock with multiple mutexes | `trylock` |

### Deadlock Avoidance with trylock

```c
// lock alone → potential deadlock
// Thread 1: holds A, waits for B
// Thread 2: holds B, waits for A  → frozen forever

// trylock → safe
pthread_mutex_lock(&A);
if (pthread_mutex_trylock(&B) != 0) {
    pthread_mutex_unlock(&A);   // release A and retry later
}
```

### ⚠️ Bad Usage of trylock

```c
// This burns CPU for no reason — worse than just using lock
while (pthread_mutex_trylock(&mutex) != 0) {
    // spinning doing nothing useful
}
```

> Only use `trylock` when you genuinely have **other useful work** to do while waiting. A spin loop with `trylock` is worse than `lock`.

---

## 10. Condition Variables

### What Is a Condition Variable?

A mutex protects data. A condition variable lets a thread **sleep until something happens** — without burning CPU.

```
mutex    → who can touch the data
condvar  → when to act on the data
```

They always work together. A condvar **requires** a mutex.

### The Problem Without Condvar

```c
// bad — busy loop burns CPU
while (balance < 50) { }  // spinning, doing nothing useful
```

### The Pattern — Always This Shape

```c
// WAITER thread
pthread_mutex_lock(&mutex);
while (condition_not_met)               // while — never if (spurious wakeups)
    pthread_cond_wait(&cond, &mutex);   // atomically: unlock + sleep
// condition is now true — safe to proceed
pthread_mutex_unlock(&mutex);

// SIGNALER thread
pthread_mutex_lock(&mutex);
// change the condition
pthread_cond_signal(&cond);             // wake one waiter
// or pthread_cond_broadcast(&cond);   // wake ALL waiters
pthread_mutex_unlock(&mutex);
```

### What `pthread_cond_wait` Does Atomically

```
1. Unlock the mutex
2. Sleep (wait for signal)
3. Re-lock the mutex when woken up
```

No gap between unlock and sleep — another thread cannot sneak in between.

### `signal` vs `broadcast`

| | Wakes | Use when |
|---|---|---|
| `pthread_cond_signal` | One waiting thread | Only one thread needs to act |
| `pthread_cond_broadcast` | All waiting threads | Multiple threads waiting on same condition |

### Why `while` and NEVER `if`

```c
// ❌ if — dangerous
if (balance < 50)
    pthread_cond_wait(&cond, &mutex);
// spurious wakeup → balance still 20 → proceeds anyway → balance goes negative 💀

// ✅ while — correct
while (balance < 50)
    pthread_cond_wait(&cond, &mutex);
// spurious wakeup → re-checks → still < 50 → goes back to sleep ✅
```

**Spurious wakeup** = OS wakes thread for no reason. Documented behavior in pthreads. `while` protects against it.

**Multiple waiters** = `broadcast` wakes both, but only one should proceed. `while` re-checks after wakeup — the one that shouldn't proceed goes back to sleep.

### Full Lifecycle

```c
pthread_cond_t cond;
pthread_cond_init(&cond, NULL);     // init once before threads
pthread_cond_wait(&cond, &mutex);   // in waiter thread
pthread_cond_signal(&cond);         // in signaler thread
pthread_cond_destroy(&cond);        // after all threads finish
```

### Bank Account Example — The Exit Condition Problem

When all deposits are done and balance is still too low, waiters sleep forever — no more signals coming.

**Fix — add total count to the while condition:**
```c
while (account->balance < 1000 && account->total_deposits < 30)
    pthread_cond_wait(&account->has_funds, &account->mutex);

if (account->balance < 1000) {  // deposits exhausted, not enough money
    pthread_mutex_unlock(&account->mutex);
    return NULL;                 // exit cleanly — don't subtract
}
account->balance -= 1000;
```

---

## 11. Producer Consumer Pattern

### The Concept

Classic threading pattern — one thread produces items, another consumes them.

```
Producer: creates items → signals consumer
Consumer: waits for items → processes them
```

### Correct Implementation

```c
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} t_mu_co;

int items = 0;  // shared resource

void *producer(void *arg) {
    t_mu_co *mc = (t_mu_co *)arg;
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mc->mutex);
        items++;
        printf("produced: %d\n", items);
        pthread_cond_signal(&mc->cond);   // signal while holding lock
        pthread_mutex_unlock(&mc->mutex);
    }
    return NULL;
}

void *consumer(void *arg) {
    t_mu_co *mc = (t_mu_co *)arg;
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mc->mutex);   // lock BEFORE while
        while (items < 1)
            pthread_cond_wait(&mc->cond, &mc->mutex);
        items--;
        printf("consumed: %d\n", items);
        pthread_mutex_unlock(&mc->mutex);
    }
    return NULL;
}
```

### Common Mistakes in This Pattern

| Mistake | Consequence |
|---|---|
| `cond_wait` before locking mutex | Undefined behavior / crash |
| `cond_signal` outside the mutex | Consumer can miss the signal |
| `if` instead of `while` | Spurious wakeup proceeds with wrong state |
| Shared variable read outside mutex | Race condition on the read |

### Why Consumer Might Not Interleave With Producer

If producer's loop is very fast, it keeps reacquiring the mutex immediately after releasing it. Consumer keeps losing the race.

**Fix — add a small sleep in producer:**
```c
pthread_mutex_unlock(&mc->mutex);
usleep(1000);  // give consumer a chance to run
```

> Releasing a mutex does NOT guarantee another thread gets it next. The OS decides.

---

## 12. pthread_exit

### What It Does

Terminates the calling thread and optionally returns a value — **without affecting other threads**.

```c
pthread_exit(void *retval);
```

### vs `return` in a Thread Function

For normal thread functions they're equivalent:
```c
return NULL;          // same effect
pthread_exit(NULL);   // same effect
```

**The difference shows in `main`:**
```c
// return 0 in main = exit() = kills ENTIRE process including all threads 💀
return 0;

// pthread_exit in main = only main thread dies, other threads keep running ✅
pthread_exit(NULL);
```

### Real Use Case — Exit from Deep in the Call Stack

```c
void deep_function(t_account *acc) {
    if (acc->balance < 0) {
        pthread_exit(NULL);  // exit thread from anywhere — no return bubbling needed
    }
}

void *worker(void *arg) {
    do_something();
    deep_function(arg);   // thread might exit from inside here
    do_more_things();     // never reached if deep_function exits
    return NULL;
}
```

Without `pthread_exit` you'd propagate error codes up through every function level.

### Returning a Value

```c
void *worker(void *arg) {
    int *result = malloc(sizeof(int));
    *result = 42;
    pthread_exit(result);  // return value retrievable via pthread_join
}

int main() {
    pthread_t t;
    void *ret;
    pthread_create(&t, NULL, worker, NULL);
    pthread_join(t, &ret);
    printf("result: %d\n", *(int *)ret);
    free(ret);
}
```

### When to Use Each

| Situation | Use |
|---|---|
| Normal exit at top of thread function | `return NULL` — simpler |
| Exit from deep inside a nested function | `pthread_exit` |
| Main should finish but threads keep running | `pthread_exit(NULL)` in main |
| Return a value from deep in the stack | `pthread_exit(result)` |

### ⚠️ Never Exit While Holding a Mutex

```c
pthread_mutex_lock(&mutex);
pthread_exit(NULL);          // ❌ mutex never unlocked — all other threads freeze forever

pthread_mutex_unlock(&mutex);
pthread_exit(NULL);          // ✅ always unlock first
```

---

## 13. Barriers

### What Is a Barrier?

A checkpoint where **all threads must arrive before any can continue**.

```
Thread 1 ──────────────►|
Thread 2 ──────►         | ← all sleep here until last thread arrives
Thread 3 ──────────────►|
                          └──► all released together
```

Last thread to arrive **wakes everyone up**.

### How to Use It

```c
pthread_barrier_t barrier;

pthread_barrier_init(&barrier, NULL, 3);  // 3 = exact number of threads that will call wait

void *worker(void *arg) {
    // PHASE 1
    do_phase_one();

    pthread_barrier_wait(&barrier);  // everyone stops here

    // PHASE 2 — only starts after ALL threads finish phase 1
    do_phase_two();
    return NULL;
}

// after all threads finish:
pthread_barrier_destroy(&barrier);
```

### How It Works Internally

The barrier keeps a counter:
```
barrier count = 3

Thread 1 arrives → count = 2 → sleep
Thread 2 arrives → count = 1 → sleep
Thread 3 arrives → count = 0 → WAKE EVERYONE UP
```

Under the hood it's a mutex + condvar + counter — barrier is a cleaner abstraction.

### Real Use Case — Image Processing

```c
// Thread 0: grayscale rows 0-249   → WAIT
// Thread 1: grayscale rows 250-499 → WAIT
// Thread 2: grayscale rows 500-749 → WAIT
// Thread 3: grayscale rows 750-999 → WAIT
//          ──── barrier ────
// ALL threads: blur their rows (needs neighbors — guaranteed ready now)
```

Without barrier: Thread 0 blurs pixel 249, needs pixel 250 as neighbor — Thread 1 hasn't converted it yet → corrupted image.

### Rules

| Rule | Why |
|---|---|
| Init count = exact threads calling `wait` | Too few → hang forever |
| Every thread MUST call `wait` | Skipping one = others wait forever |
| Can be reused | Resets automatically after each release |
| Always `destroy` after use | Resource hygiene |

### Barrier vs Other Tools

| Tool | Question it answers |
|---|---|
| `mutex` | Who can touch this data? |
| `cond_var` | Is this condition true yet? |
| `barrier` | Are we ALL done with this phase? |

> Use a barrier whenever **phase 2 reads data that phase 1 writes**, and phase 1 is split across multiple threads.

---

## 14. Codexion Context

### Thread Structure

```
Main thread
├── Coder thread 0  ──► coder_routine(&coders[0])
├── Coder thread 1  ──► coder_routine(&coders[1])
├── Coder thread 2  ──► coder_routine(&coders[2])
├── Coder thread 3  ──► coder_routine(&coders[3])
└── Monitor thread  ──► monitor_routine(sim)

Total: N coders + 1 monitor = N+1 threads
```

### What Each Thread Owns vs Shares

| | **Private (per-thread stack)** | **Shared (heap — needs mutex)** |
|--|-------------------------------|--------------------------------|
| Coder thread | local loop counter | dongle, compile count, stopped flag |
| Monitor thread | local check variables | all coder states, sim state |

### Key Mutexes You'll Need

```c
pthread_mutex_t dongle_mutex;    // protect dongle acquisition
pthread_mutex_t log_mutex;       // protect printf output
pthread_mutex_t state_mutex;     // protect sim->stopped
```

---

## 15. Cheatsheet

```c
// ── THREAD ──────────────────────────────────────────
pthread_t          t;
pthread_create(&t, NULL, func, arg);    // spawn
pthread_join(t, &ret);                  // wait + get return value
pthread_exit(result);                   // exit thread from anywhere

// Thread function shape
void *func(void *arg) { return NULL; }

// ── MUTEX ───────────────────────────────────────────
pthread_mutex_t    m;
pthread_mutex_init(&m, NULL);           // init once
pthread_mutex_lock(&m);                 // lock — blocks until acquired
pthread_mutex_trylock(&m);              // try lock — returns 0 if acquired, non-zero if busy
    /* critical section */
pthread_mutex_unlock(&m);               // unlock
pthread_mutex_destroy(&m);             // cleanup

// ── CONDITION VARIABLE ──────────────────────────────
pthread_cond_t     c;
pthread_cond_init(&c, NULL);            // init once
pthread_mutex_lock(&m);
while (!condition)                      // always while — never if
    pthread_cond_wait(&c, &m);          // atomically: unlock + sleep + relock
pthread_mutex_unlock(&m);

pthread_cond_signal(&c);                // wake one waiter (inside lock)
pthread_cond_broadcast(&c);            // wake all waiters (inside lock)
pthread_cond_destroy(&c);              // cleanup

// ── BARRIER ─────────────────────────────────────────
pthread_barrier_t  b;
pthread_barrier_init(&b, NULL, N);      // N = exact number of threads
pthread_barrier_wait(&b);              // all threads block until N arrive
pthread_barrier_destroy(&b);           // cleanup

// ── TIMESTAMP (milliseconds) ────────────────────────
long long now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL + tv.tv_usec / 1000);
}
```

---

## 16. Self-Test Questions

Test yourself before reviewing. Answer mentally, then check.

**Threads**
- [ ] Why can't you simulate 4 concurrent coders without threads?
- [ ] What is the difference between a process and a thread?
- [ ] What happens if you forget `pthread_join`?
- [ ] Why is passing `&i` to `pthread_create` in a loop a bug?

**Race Conditions**
- [ ] Why is `counter++` not atomic?
- [ ] What are the 3 CPU steps hidden in `counter++`?
- [ ] Why do race conditions sometimes disappear when you add `printf`?
- [ ] In Codexion, two coders check `dongle->available == 1` simultaneously and both see `true` — what goes wrong?

**Mutex**
- [ ] What does `pthread_mutex_lock` do when another thread already holds the mutex?
- [ ] What happens if you forget to call `pthread_mutex_unlock`?
- [ ] Why should critical sections be as short as possible?

**Deadlock**
- [ ] Name Coffman's 4 conditions for deadlock.
- [ ] How does "always lock in the same order" prevent deadlock?
- [ ] Which of the 4 conditions does it break?

**lock vs trylock**
- [ ] What does `pthread_mutex_lock` do when the mutex is already taken?
- [ ] Does a sleeping thread (waiting on lock) waste CPU? Why not?
- [ ] When does `trylock` make things worse instead of better?
- [ ] How can `trylock` help avoid deadlock with two mutexes?

**Parallel Sum**
- [ ] Why does parallelism NOT help with 10 elements but helps with 10 million?
- [ ] What is the bug if you use one `malloc` outside the loop for all threads?
- [ ] Why does `pthread_join` need `&ret` instead of `ret`?
- [ ] Why must `free(ret)` be inside the join loop, not after it?

**Condition Variables**
- [ ] What does `pthread_cond_wait` do atomically?
- [ ] Why must the mutex be locked BEFORE calling `cond_wait`?
- [ ] What is a spurious wakeup and why does it force us to use `while`?
- [ ] When do you use `signal` vs `broadcast`?
- [ ] Why must `cond_signal` be called while holding the mutex?
- [ ] What happens if all deposits finish but the withdrawer is still waiting?

**Producer Consumer**
- [ ] Why does the consumer sometimes not run until producer fully finishes?
- [ ] Does releasing a mutex guarantee another thread gets it next?
- [ ] What is the fix when a thread keeps winning the mutex race?

**pthread_exit**
- [ ] What is the difference between `return 0` in main and `pthread_exit(NULL)` in main?
- [ ] When is `pthread_exit` more useful than `return`?
- [ ] What happens if you call `pthread_exit` while holding a mutex?

**Barriers**
- [ ] What happens if only 2 out of 3 threads call `pthread_barrier_wait`?
- [ ] What problem does a barrier solve that a mutex cannot?
- [ ] Can a barrier be reused after all threads pass it?
- [ ] Give a real example where a barrier is necessary.

---

*Last updated from mentoring session · May 2026*

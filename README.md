# C++ Multithreaded Task Scheduler

A small C++17 library and demo application that accepts `Task` objects and
executes them on a fixed-size pool of worker threads, backed by a
thread-safe queue guarded with a mutex and condition variable.

## Overview

```
                +------------------+
                |   Task Scheduler |
                +--------+---------+
                         |
                    Task Queue
                         |
             +-----------+-----------+
             |           |           |
          Worker 1    Worker 2    Worker 3
             |           |           |
           Task A      Task B      Task C
```

Users submit `Task` objects → tasks enter a thread-safe queue → idle worker
threads pick them up → execute them → the scheduler manages the pool's
lifecycle (start, submit, graceful shutdown).

## Features

- Polymorphic `Task` abstraction (`execute()`), with `PrintTask`,
  `CalculationTask`, and `SleepTask` as concrete examples.
- Fixed-size thread pool (`TaskScheduler`) built on `std::thread`.
- Thread-safe task queue protected by `std::mutex` + `std::condition_variable`
  (no busy-waiting — idle workers block until woken).
- Graceful shutdown: stops accepting new work but drains everything already
  queued before joining worker threads.
- RAII: the destructor shuts the pool down automatically if the caller
  forgot to.
- Per-task exception isolation: a task that throws is logged and the worker
  keeps running.
- Thread-safe logger for readable interleaved output.
- Unit tests (GoogleTest) and a measured concurrency benchmark.

## Architecture

- **`Task`** (`include/Task.h`) — abstract base with a single `execute()`
  method. `PrintTask`, `CalculationTask`, and `SleepTask` are concrete
  implementations demonstrating I/O-bound, CPU-bound, and blocking work.
- **`TaskScheduler`** (`include/TaskScheduler.h`, `src/TaskScheduler.cpp`) —
  owns the worker threads and the task queue. `submit()` pushes a task and
  notifies one waiting worker; each worker loops: wait for work → pop →
  execute → repeat.
- **`Logger`** (`include/Logger.h`) — mutex-guarded `std::cout` wrapper so
  concurrent workers don't produce interleaved/garbled output.

## Technologies

C++17, STL (`std::thread`, `std::mutex`, `std::condition_variable`,
`std::queue`, `std::unique_ptr`, `std::function`, `std::chrono`), CMake,
GoogleTest.

## Building

Requires a C++17 compiler, CMake ≥ 3.14, and GoogleTest discoverable via
`find_package(GTest CONFIG REQUIRED)` (e.g. `pacman -S mingw-w64-ucrt-x86_64-gtest`
on MSYS2/UCRT64, or any system package manager / vcpkg install of GTest).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

(Any CMake generator works — Ninja, Makefiles, or a Visual Studio solution;
`-G Ninja` above is just what was used to build and verify this project.)

## Running

```bash
./build/demo.exe        # lifecycle demo: submit a mix of tasks, then shutdown
./build/benchmark.exe   # measured speedup with 1/2/4/8 worker threads
```

## Testing

```bash
ctest --test-dir build --output-on-failure
```

11 tests covering: single/multiple task execution, real concurrency across
workers, graceful draining on shutdown, rejecting `submit()` after shutdown,
shutting down an empty scheduler, a throwing task not killing the pool,
RAII shutdown via the destructor, single-worker serial correctness, tasks
actually landing on different threads, and `pendingTaskCount()` accuracy.

All 11 pass:

```
100% tests passed out of 11
Total Test time (real) =   1.73 sec
```

## Design Decisions

**Why `std::mutex`?**
The task queue is shared: the submitting thread pushes while multiple
worker threads pop concurrently. Without a mutex, concurrent
`std::queue` access is undefined behavior (data races on internal state).

**Why `std::condition_variable`?**
To avoid busy-waiting. Without it, idle workers would have to poll the
queue in a loop, burning CPU for no reason. `cv_.wait(lock, predicate)`
puts a worker to sleep until `submit()` or `shutdown()` calls
`notify_one()`/`notify_all()`.

**Why `std::unique_ptr<Task>`?**
Ownership of a task is single and linear: it moves from the caller, into
the queue, into a worker, and is destroyed after `execute()` returns.
`unique_ptr` expresses that model directly and gives free, automatic
cleanup — no manual `delete`, no leak if a task throws.

**Why a thread pool instead of a thread per task?**
Creating and destroying an OS thread has real overhead. A fixed pool of
reusable worker threads amortizes that cost — thread creation happens once
at `start()`, not once per task.

**What happens to queued tasks on `shutdown()`?**
`shutdown()` sets a `stopping_` flag and wakes every worker, but each
worker keeps draining the queue until it's *both* empty *and* `stopping_`
is true. So `shutdown()` is graceful by default — everything submitted
before it was called still runs; nothing is silently dropped. There's no
separate "cancel immediately" mode, keeping the shutdown contract simple
and predictable.

**How are deadlocks avoided?**
The queue mutex is only ever held for the brief critical sections around
push/pop — task `execute()` always runs *outside* the lock, so a
long-running or blocking task can't stall submission or other workers.

**Why isolate exceptions per task?**
A worker thread is a limited, reusable resource. If a single misbehaving
task's exception propagated out of the worker loop, that thread would
terminate (`std::terminate` if unhandled, or simply exit the loop),
permanently shrinking the pool. Catching around `task->execute()` keeps
every worker alive for the life of the scheduler.

## Concurrency Model

One shared queue, N long-lived worker threads, mutex + condition variable
for coordination, atomic `stopping_` flag for the shutdown signal. This is
a classic bounded producer/multi-consumer pattern — `submit()` is the
producer side, worker threads are the consumers.

## Benchmark

10 tasks, each sleeping 1 second, run against increasing pool sizes
(measured on this machine with `std::chrono::steady_clock`, `examples/benchmark.cpp`):

| Workers | Elapsed (s) |
|---------|-------------|
| 1       | 10.35       |
| 2       | 5.92        |
| 4       | 3.02        |
| 8       | 2.01        |

Roughly linear speedup up to 4 workers; diminishing returns at 8 (more
workers than tasks-per-round for this workload, plus fixed thread-creation
and scheduling overhead).

## Future Improvements

- Priority queue instead of FIFO for task ordering.
- Work-stealing between per-worker queues to reduce contention at high
  worker counts.
- A `std::future`-returning `submit()` overload for tasks with results.
- Configurable "shutdown now" mode that discards queued-but-not-started
  tasks instead of draining them.

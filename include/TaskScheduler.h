#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Task.h"

// Fixed-size thread pool that executes submitted Tasks.
//
// Lifecycle: construct -> start() -> submit() any number of times -> shutdown().
// shutdown() is graceful: it stops accepting new tasks but lets worker
// threads drain everything already queued before joining them. If the
// caller never calls shutdown() explicitly, the destructor does it (RAII),
// so a TaskScheduler never leaks running threads.
class TaskScheduler {
public:
    explicit TaskScheduler(std::size_t numThreads = std::thread::hardware_concurrency());
    ~TaskScheduler();

    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;

    // Spawns the worker threads. Safe to call at most once.
    void start();

    // Enqueues a task for execution by some worker thread.
    // Throws std::runtime_error if called after shutdown() has begun.
    void submit(std::unique_ptr<Task> task);

    // Stops accepting new tasks, lets workers drain the queue, then joins
    // every worker thread. Safe to call multiple times (subsequent calls
    // are no-ops).
    void shutdown();

    // Number of tasks currently sitting in the queue (not yet picked up).
    std::size_t pendingTaskCount() const;

private:
    void workerLoop();

    std::size_t numThreads_;
    std::vector<std::thread> workers_;

    std::queue<std::unique_ptr<Task>> taskQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable cv_;

    std::atomic<bool> stopping_{false};
    bool started_ = false;
};

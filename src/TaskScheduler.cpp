#include "TaskScheduler.h"

#include <algorithm>
#include <stdexcept>

#include "Logger.h"

TaskScheduler::TaskScheduler(std::size_t numThreads)
    : numThreads_(std::max<std::size_t>(1, numThreads)) {}

TaskScheduler::~TaskScheduler() { shutdown(); }

void TaskScheduler::start() {
    if (started_) {
        return;
    }
    started_ = true;
    workers_.reserve(numThreads_);
    for (std::size_t i = 0; i < numThreads_; ++i) {
        workers_.emplace_back(&TaskScheduler::workerLoop, this);
    }
}

void TaskScheduler::submit(std::unique_ptr<Task> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stopping_) {
            throw std::runtime_error("TaskScheduler::submit called after shutdown");
        }
        taskQueue_.push(std::move(task));
    }
    cv_.notify_one();
}

void TaskScheduler::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }
    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

std::size_t TaskScheduler::pendingTaskCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return taskQueue_.size();
}

void TaskScheduler::workerLoop() {
    while (true) {
        std::unique_ptr<Task> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this] { return stopping_ || !taskQueue_.empty(); });

            if (taskQueue_.empty()) {
                // Only reachable when stopping_ is true and the queue is
                // drained -- nothing left to do.
                return;
            }

            task = std::move(taskQueue_.front());
            taskQueue_.pop();
        }

        try {
            task->execute();
        } catch (const std::exception& e) {
            Logger::error(std::string("Task threw an exception: ") + e.what());
        } catch (...) {
            Logger::error("Task threw a non-std::exception value");
        }
    }
}

#pragma once

#include <chrono>
#include <thread>

#include "Task.h"

// Task that sleeps for a fixed duration, used to simulate blocking work in
// tests and the benchmark (makes concurrency effects observable via timing).
class SleepTask : public Task {
public:
    explicit SleepTask(std::chrono::milliseconds duration) : duration_(duration) {}

    void execute() override { std::this_thread::sleep_for(duration_); }

private:
    std::chrono::milliseconds duration_;
};

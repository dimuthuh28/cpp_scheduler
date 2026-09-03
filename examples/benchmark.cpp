#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "SleepTask.h"
#include "TaskScheduler.h"

namespace {
constexpr int kNumTasks = 10;
constexpr auto kTaskDuration = std::chrono::milliseconds(1000);

double runWithWorkers(std::size_t numWorkers) {
    TaskScheduler scheduler(numWorkers);
    scheduler.start();

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kNumTasks; ++i) {
        scheduler.submit(std::make_unique<SleepTask>(kTaskDuration));
    }
    scheduler.shutdown();  // blocks until all tasks are drained
    auto end = std::chrono::steady_clock::now();

    return std::chrono::duration<double>(end - start).count();
}
}  // namespace

int main() {
    std::cout << kNumTasks << " tasks, each sleeping "
              << std::chrono::duration<double>(kTaskDuration).count() << "s\n\n";
    std::cout << std::left << std::setw(10) << "Workers" << "Elapsed (s)\n";
    std::cout << "----------------------\n";

    for (std::size_t workers : {1u, 2u, 4u, 8u}) {
        double elapsed = runWithWorkers(workers);
        std::cout << std::left << std::setw(10) << workers << std::fixed
                   << std::setprecision(2) << elapsed << "\n";
    }

    return 0;
}

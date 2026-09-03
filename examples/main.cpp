#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include "CalculationTask.h"
#include "Logger.h"
#include "PrintTask.h"
#include "SleepTask.h"
#include "Task.h"
#include "TaskScheduler.h"

int main() {
    Logger::info("Starting scheduler with 4 worker threads");
    TaskScheduler scheduler(4);
    scheduler.start();

    for (int i = 0; i < 5; ++i) {
        scheduler.submit(std::make_unique<PrintTask>("Hello from task #" + std::to_string(i)));
    }

    scheduler.submit(std::make_unique<CalculationTask>(1000));
    scheduler.submit(std::make_unique<SleepTask>(std::chrono::milliseconds(200)));

    // A task that throws -- demonstrates the pool surviving a bad task
    // instead of one worker thread silently dying.
    class ThrowingTask : public Task {
    public:
        void execute() override { throw std::runtime_error("simulated task failure"); }
    };
    scheduler.submit(std::make_unique<ThrowingTask>());

    scheduler.submit(std::make_unique<PrintTask>("Final task"));

    Logger::info("All tasks submitted, shutting down (will drain queue first)");
    scheduler.shutdown();
    Logger::info("Scheduler shut down cleanly");

    return 0;
}

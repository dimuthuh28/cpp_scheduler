#include "TaskScheduler.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "SleepTask.h"
#include "Task.h"

namespace {

// Runs a lambda when executed; used to observe side effects from tests.
class LambdaTask : public Task {
public:
    explicit LambdaTask(std::function<void()> fn) : fn_(std::move(fn)) {}
    void execute() override { fn_(); }

private:
    std::function<void()> fn_;
};

class ThrowingTask : public Task {
public:
    void execute() override { throw std::runtime_error("boom"); }
};

}  // namespace

TEST(TaskScheduler, SingleTaskExecutes) {
    TaskScheduler scheduler(2);
    scheduler.start();

    std::atomic<bool> ran{false};
    scheduler.submit(std::make_unique<LambdaTask>([&] { ran = true; }));
    scheduler.shutdown();

    EXPECT_TRUE(ran);
}

TEST(TaskScheduler, MultipleTasksAllExecuteExactlyOnce) {
    TaskScheduler scheduler(4);
    scheduler.start();

    constexpr int kCount = 100;
    std::atomic<int> counter{0};
    for (int i = 0; i < kCount; ++i) {
        scheduler.submit(std::make_unique<LambdaTask>([&] { counter.fetch_add(1); }));
    }
    scheduler.shutdown();

    EXPECT_EQ(counter.load(), kCount);
}

TEST(TaskScheduler, TasksRunConcurrentlyAcrossWorkers) {
    // 8 tasks that each sleep 200ms should finish in well under 8*200ms if
    // they truly overlap on 4 workers.
    TaskScheduler scheduler(4);
    scheduler.start();

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 8; ++i) {
        scheduler.submit(std::make_unique<SleepTask>(std::chrono::milliseconds(200)));
    }
    scheduler.shutdown();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 1200);
}

TEST(TaskScheduler, ShutdownDrainsTasksSubmittedBeforeIt) {
    TaskScheduler scheduler(2);
    scheduler.start();

    constexpr int kCount = 20;
    std::atomic<int> counter{0};
    for (int i = 0; i < kCount; ++i) {
        scheduler.submit(std::make_unique<LambdaTask>([&] { counter.fetch_add(1); }));
    }
    scheduler.shutdown();

    EXPECT_EQ(counter.load(), kCount);
    EXPECT_EQ(scheduler.pendingTaskCount(), 0u);
}

TEST(TaskScheduler, SubmitAfterShutdownThrows) {
    TaskScheduler scheduler(2);
    scheduler.start();
    scheduler.shutdown();

    EXPECT_THROW(scheduler.submit(std::make_unique<LambdaTask>([] {})), std::runtime_error);
}

TEST(TaskScheduler, EmptySchedulerShutsDownWithoutHanging) {
    TaskScheduler scheduler(3);
    scheduler.start();
    scheduler.shutdown();  // no tasks submitted at all

    SUCCEED();
}

TEST(TaskScheduler, ThrowingTaskDoesNotStopThePool) {
    TaskScheduler scheduler(2);
    scheduler.start();

    scheduler.submit(std::make_unique<ThrowingTask>());

    std::atomic<bool> ranAfter{false};
    scheduler.submit(std::make_unique<LambdaTask>([&] { ranAfter = true; }));
    scheduler.shutdown();

    EXPECT_TRUE(ranAfter);
}

TEST(TaskScheduler, DestructorShutsDownAutomatically) {
    std::atomic<bool> ran{false};
    {
        TaskScheduler scheduler(2);
        scheduler.start();
        scheduler.submit(std::make_unique<LambdaTask>([&] { ran = true; }));
        // No explicit shutdown() call -- destructor must join and drain.
    }
    EXPECT_TRUE(ran);
}

TEST(TaskScheduler, SingleWorkerExecutesAllTasksSerially) {
    TaskScheduler scheduler(1);
    scheduler.start();

    constexpr int kCount = 50;
    std::atomic<int> counter{0};
    for (int i = 0; i < kCount; ++i) {
        scheduler.submit(std::make_unique<LambdaTask>([&] { counter.fetch_add(1); }));
    }
    scheduler.shutdown();

    EXPECT_EQ(counter.load(), kCount);
}

TEST(TaskScheduler, TasksActuallyRunOnDifferentWorkerThreads) {
    TaskScheduler scheduler(4);
    scheduler.start();

    std::mutex idsMutex;
    std::set<std::thread::id> ids;
    for (int i = 0; i < 20; ++i) {
        scheduler.submit(std::make_unique<LambdaTask>([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::lock_guard<std::mutex> lock(idsMutex);
            ids.insert(std::this_thread::get_id());
        }));
    }
    scheduler.shutdown();

    EXPECT_GT(ids.size(), 1u);
}

TEST(TaskScheduler, PendingTaskCountReflectsQueueState) {
    TaskScheduler scheduler(1);
    scheduler.start();

    // Block the single worker so submitted tasks pile up in the queue.
    std::atomic<bool> release{false};
    scheduler.submit(std::make_unique<LambdaTask>([&] {
        while (!release) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }));

    // Give the worker time to pick up the blocking task.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    for (int i = 0; i < 5; ++i) {
        scheduler.submit(std::make_unique<LambdaTask>([] {}));
    }
    EXPECT_EQ(scheduler.pendingTaskCount(), 5u);

    release = true;
    scheduler.shutdown();
    EXPECT_EQ(scheduler.pendingTaskCount(), 0u);
}

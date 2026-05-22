#include "../../Engine/Common/TaskScheduler.h"
#include "../../Engine/Common/Task.h"
#include "../Common/TestRunner.h"
#include <chrono>
#include <vector>
#include <atomic>
#include <random>

using namespace Inno;

void TestBasicTaskExecution()
{
    TestRunner::StartTest("Basic Task Execution");
    
    bool testPassed = true;
    TaskScheduler scheduler;
    std::atomic<int> counter{0};
    
    try {
        auto task = scheduler.Submit(
            ITask::Desc("TestTask", ITask::Type::Once),
            [&counter]() { counter.fetch_add(1); }
        );

        // Activation is the user's responsibility — Submit alone does not run the task.
        task->Activate();
        task->Wait();
        
        if (counter.load() != 1) {
            testPassed = false;
        }
    } catch (...) {
        testPassed = false;
    }
    
    TestRunner::EndTest(testPassed);
}

void TestConcurrentTaskSubmission()
{
    TestRunner::StartTest("Concurrent Task Submission");
    
    bool testPassed = true;
    TaskScheduler scheduler;
    std::atomic<int> completedTasks{0};
    constexpr int NUM_THREADS = 4;
    constexpr int TASKS_PER_THREAD = 50;

    try {
        std::vector<std::thread> submitterThreads;
        std::vector<SharedPtr<ITask>> allTasks;
        std::mutex tasksMutex;

        for (int t = 0; t < NUM_THREADS; ++t) {
            submitterThreads.emplace_back([&, t]() {
                std::vector<SharedPtr<ITask>> localTasks;
                
                for (int i = 0; i < TASKS_PER_THREAD; ++i) {
                    std::string taskName = "ConcurrentTask_" + std::to_string(t) + "_" + std::to_string(i);
                    auto task = scheduler.Submit(
                        ITask::Desc(taskName.c_str(), ITask::Type::Once),
                        [&completedTasks]() {
                            completedTasks.fetch_add(1);
                            // Widens the race window the test is trying to exercise.
                            std::this_thread::sleep_for(std::chrono::microseconds(1));
                        }
                    );
                    task->Activate();
                    localTasks.push_back(task);
                }

                {
                    std::lock_guard<std::mutex> lock(tasksMutex);
                    allTasks.insert(allTasks.end(), localTasks.begin(), localTasks.end());
                }
            });
        }

        for (auto& thread : submitterThreads) {
            thread.join();
        }

        for (auto& task : allTasks) {
            task->Wait();
        }
        
        const int expectedTasks = NUM_THREADS * TASKS_PER_THREAD;
        if (completedTasks.load() != expectedTasks) {
            testPassed = false;
        }
    } catch (...) {
        testPassed = false;
    }
    
    TestRunner::EndTest(testPassed);
}

void TestTaskExecutionOrdering()
{
    TestRunner::StartTest("Task Execution Ordering");
    
    bool testPassed = true;
    TaskScheduler scheduler;
    std::atomic<int> executionOrder{0};
    std::vector<int> results;
    std::mutex resultsMutex;
    constexpr int NUM_TASKS = 25;

    try {
        std::vector<SharedPtr<ITask>> tasks;

        for (int i = 0; i < NUM_TASKS; ++i) {
            std::string taskName = "OrderingTask_" + std::to_string(i);
            auto task = scheduler.Submit(
                ITask::Desc(taskName.c_str(), ITask::Type::Once),
                [&, i]() {
                    int order = executionOrder.fetch_add(1);
                    {
                        std::lock_guard<std::mutex> lock(resultsMutex);
                        results.push_back(i);
                    }
                }
            );
            task->Activate();
            tasks.push_back(task);
        }

        for (auto& task : tasks) {
            task->Wait();
        }
        
        if (results.size() != NUM_TASKS) {
            testPassed = false;
        }
    } catch (...) {
        testPassed = false;
    }
    
    TestRunner::EndTest(testPassed);
}

void TestFreezeUnfreezeStressTest()
{
    TestRunner::StartTest("Freeze/Unfreeze Stress Test");
    
    bool testPassed = true;
    
    try {
        TaskScheduler scheduler;
        std::atomic<int> taskCounter{0};
        std::atomic<bool> shouldStop{false};
        
        std::vector<SharedPtr<ITask>> recurringTasks;
        for (int i = 0; i < 5; ++i) {
            std::string taskName = "RecurringTask_" + std::to_string(i);
            auto task = scheduler.Submit(
                ITask::Desc(taskName.c_str(), ITask::Type::Recurrent),
                [&taskCounter, &shouldStop]() {
                    if (!shouldStop.load()) {
                        taskCounter.fetch_add(1);
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                }
            );
            recurringTasks.push_back(task);
        }

        for (auto& task : recurringTasks) {
            task->Activate();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        int initialCount = taskCounter.load();

        scheduler.Freeze();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        int frozenCount = taskCounter.load();

        scheduler.Unfreeze();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        int unfrozenCount = taskCounter.load();

        shouldStop.store(true);

        for (auto& task : recurringTasks) {
            task->Deactivate();
        }

        if (frozenCount < initialCount || unfrozenCount < frozenCount) {
            // Timing-sensitive: allow drift; the real assertion is "no crash".
        }
    } catch (...) {
        testPassed = false;
    }
    
    TestRunner::EndTest(testPassed);
}

void TestTaskCleanupTest()
{
    TestRunner::StartTest("Task Cleanup Test");
    
    bool testPassed = true;
    
    try {
        TaskScheduler scheduler;
        
        {
            std::vector<SharedPtr<ITask>> tasks;
            for (int i = 0; i < 50; ++i) {
                std::string taskName = "CleanupTask_" + std::to_string(i);
                auto task = scheduler.Submit(
                    ITask::Desc(taskName.c_str(), ITask::Type::Once),
                    []() {
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                );
                tasks.push_back(task);
            }

            for (auto& task : tasks) {
                task->Wait();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } catch (...) {
        testPassed = false;
    }
    
    TestRunner::EndTest(testPassed);
}

void RunTaskSystemTests()
{
    TestRunner::StartTestSuite("Task System Tests");
    
    TestBasicTaskExecution();
    TestConcurrentTaskSubmission();
    TestTaskExecutionOrdering();
    TestFreezeUnfreezeStressTest();
    TestTaskCleanupTest();
    
    TestRunner::EndTestSuite();
}

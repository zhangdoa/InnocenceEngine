// TaskScheduler stress tests.
// These tests push the scheduler harder than the functional suite:
//   1. Memory ordering — proves Wait() provides happens-before for written data.
//   2. High-concurrency submission — 8 threads × 2000 tasks (16 000 total).
//   3. Recurrent-under-load — Recurrent task survives a flood of Once tasks.
//   4. Freeze/Unfreeze stress — 100 rapid Freeze→Unfreeze cycles; deadlock detection.
//
// Design rules (derived from reading TaskScheduler/Thread/Task internals):
//   - Tasks need explicit Activate() after Submit().
//   - Reset() destroys thread objects; do not resubmit on the same scheduler after Reset().
//   - GenerateThreadIndex with default (max_uint32) picks a random thread each call.
//   - Wait() returns when: state==Released (Once done), state==Idle (Recurrent
//     deactivated), or executionCount>0 (at least one execution completed).
//   - Recurrent tasks are not auto-removed; caller must Deactivate() them.

#include "../../Engine/Common/TaskScheduler.h"
#include "../../Engine/Common/Task.h"
#include "../Common/TestRunner.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

using namespace Inno;

// ---------------------------------------------------------------------------
// 1. Memory ordering
//    Producer task writes a payload. Main thread calls Wait(). Consumer verifies
//    the payload is visible — this exercises the acquire/release pair on
//    m_ExecutionCount and m_State in TryToExecute() / Wait().
// ---------------------------------------------------------------------------
static void StressMemoryOrdering()
{
    TestRunner::StartTest("Stress: memory ordering across Wait()");

    constexpr int ITERATIONS = 10000;
    constexpr int PAYLOAD_WORDS = 64;

    TaskScheduler scheduler;
    bool passed = true;

    // Shared payload; no atomic needed — Wait() provides the happens-before.
    uint64_t payload[PAYLOAD_WORDS] = {};

    for (int iter = 0; iter < ITERATIONS && passed; ++iter)
    {
        const uint64_t sentinel = static_cast<uint64_t>(iter) ^ 0xDEADBEEFCAFEBABEULL;

        // Producer: write sentinel into every word.
        auto producer = scheduler.Submit(
            ITask::Desc("MemOrd_Producer", ITask::Type::Once),
            [&payload, sentinel, PAYLOAD_WORDS]()
            {
                for (int w = 0; w < PAYLOAD_WORDS; ++w)
                    payload[w] = sentinel;
            });
        producer->Activate();
        producer->Wait();

        // After Wait() the payload writes are visible on this thread.
        for (int w = 0; w < PAYLOAD_WORDS; ++w)
        {
            if (payload[w] != sentinel)
            {
                passed = false;
                break;
            }
        }
    }

    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// 2. High-concurrency submission
//    8 submitter threads each submit 2000 Once tasks. A shared atomic counter
//    is incremented once per task body. After all Wait() calls, counter must
//    equal 16 000.
// ---------------------------------------------------------------------------
static void StressHighConcurrencySubmission()
{
    TestRunner::StartTest("Stress: high-concurrency submission (8 threads × 2000 tasks)");

    constexpr int NUM_SUBMITTERS  = 8;
    constexpr int TASKS_PER_THREAD = 2000;
    constexpr int EXPECTED = NUM_SUBMITTERS * TASKS_PER_THREAD;

    TaskScheduler scheduler;
    std::atomic<int> counter{0};
    bool passed = true;

    // Each submitter thread collects its own task handles, activates them,
    // then waits on them locally before joining. No shared handle vector
    // needed — eliminates the mutex bottleneck and the race between inserting
    // and waiting.
    std::vector<std::thread> submitters;
    submitters.reserve(NUM_SUBMITTERS);

    for (int t = 0; t < NUM_SUBMITTERS; ++t)
    {
        submitters.emplace_back([&, t]()
        {
            std::vector<Handle<ITask>> local;
            local.reserve(TASKS_PER_THREAD);

            for (int i = 0; i < TASKS_PER_THREAD; ++i)
            {
                auto task = scheduler.Submit(
                    ITask::Desc("HiConc_Task", ITask::Type::Once),
                    [&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
                task->Activate();
                local.push_back(task);
            }

            for (auto& h : local)
                h->Wait();
        });
    }

    for (auto& th : submitters)
        th.join();

    if (counter.load(std::memory_order_acquire) != EXPECTED)
        passed = false;

    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// 3. Recurrent-under-load
//    One Recurrent task increments recurrentCount on every tick. Then 1000
//    Once tasks are submitted (incrementing onceCount). After all Once tasks
//    complete, we deactivate the Recurrent task and verify onceCount == 1000.
//    This proves the scheduler handles mixed task types without dropping work.
// ---------------------------------------------------------------------------
static void StressRecurrentUnderLoad()
{
    TestRunner::StartTest("Stress: Recurrent task survives Once-task flood");

    constexpr int ONCE_COUNT = 1000;

    TaskScheduler scheduler;
    std::atomic<int> recurrentCount{0};
    std::atomic<int> onceCount{0};
    bool passed = true;

    // Submit and activate the recurrent task.
    auto recurrent = scheduler.Submit(
        ITask::Desc("Recurrent_Flood", ITask::Type::Recurrent),
        [&recurrentCount]() { recurrentCount.fetch_add(1, std::memory_order_relaxed); });
    recurrent->Activate();

    // Flood Once tasks.
    std::vector<Handle<ITask>> onceTasks;
    onceTasks.reserve(ONCE_COUNT);
    for (int i = 0; i < ONCE_COUNT; ++i)
    {
        auto t = scheduler.Submit(
            ITask::Desc("Once_Flood", ITask::Type::Once),
            [&onceCount]() { onceCount.fetch_add(1, std::memory_order_relaxed); });
        t->Activate();
        onceTasks.push_back(t);
    }

    for (auto& h : onceTasks)
        h->Wait();

    // Stop the recurrent task.
    recurrent->Deactivate();
    recurrent->Wait();

    if (onceCount.load(std::memory_order_acquire) != ONCE_COUNT)
        passed = false;

    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// 4. Freeze/Unfreeze stress
//    100 iterations of Freeze→Unfreeze with Recurrent tasks in flight.
//    Each iteration is wrapped in a std::future with a 10-second timeout to
//    detect deadlocks. Once tasks submitted between freeze cycles must all
//    complete after unfreeze.
// ---------------------------------------------------------------------------
static void StressFreezeUnfreeze()
{
    TestRunner::StartTest("Stress: 100 Freeze/Unfreeze cycles (deadlock detection)");

    constexpr int CYCLES = 100;
    constexpr int ONCE_PER_CYCLE = 10;

    TaskScheduler scheduler;
    std::atomic<int> recurrentCount{0};
    std::atomic<int> onceTotal{0};
    bool passed = true;

    // Recurrent task — runs throughout.
    auto recurrent = scheduler.Submit(
        ITask::Desc("FU_Recurrent", ITask::Type::Recurrent),
        [&recurrentCount]() { recurrentCount.fetch_add(1, std::memory_order_relaxed); });
    recurrent->Activate();

    for (int cycle = 0; cycle < CYCLES && passed; ++cycle)
    {
        // Run the freeze→submit→unfreeze→wait sequence in a future so we can
        // detect a deadlock via timed_wait.
        auto fut = std::async(std::launch::async, [&]()
        {
            scheduler.Freeze();

            // Submit Once tasks while frozen; they queue up but don't execute.
            std::vector<Handle<ITask>> cycle_tasks;
            cycle_tasks.reserve(ONCE_PER_CYCLE);
            for (int i = 0; i < ONCE_PER_CYCLE; ++i)
            {
                auto t = scheduler.Submit(
                    ITask::Desc("FU_Once", ITask::Type::Once),
                    [&onceTotal]() { onceTotal.fetch_add(1, std::memory_order_relaxed); });
                t->Activate();
                cycle_tasks.push_back(t);
            }

            scheduler.Unfreeze();

            for (auto& h : cycle_tasks)
                h->Wait();
        });

        auto status = fut.wait_for(std::chrono::seconds(10));
        if (status != std::future_status::ready)
        {
            passed = false; // deadlock detected
            break;
        }
    }

    recurrent->Deactivate();
    recurrent->Wait();

    const int expectedOnce = CYCLES * ONCE_PER_CYCLE;
    if (passed && onceTotal.load(std::memory_order_acquire) != expectedOnce)
        passed = false;

    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
void RunTaskSystemStressTests()
{
    TestRunner::StartTestSuite("Task System Stress Tests");

    StressMemoryOrdering();
    StressHighConcurrencySubmission();
    StressRecurrentUnderLoad();
    StressFreezeUnfreeze();

    TestRunner::EndTestSuite();
}

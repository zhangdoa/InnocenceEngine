#include "Task.h"

#include "LogService.h"
#include "Timer.h"

#include "../Engine.h"

using namespace Inno;

#include <chrono>

void ITask::Activate()
{
    auto start = Timer::GetCurrentTimeFromEpoch();
    const auto timeout = 5000;

    while (true)
    {
        State expected = m_State.load(std::memory_order_acquire);
        if (expected == State::Done || expected == State::Executing || expected == State::Waiting)
        {
            Log(Verbose, "Task: \"", GetName(), "\" has already been activated.");
            return;
        }

        if (m_State.compare_exchange_weak(expected, State::Waiting, std::memory_order_acq_rel))
        {
            Log(Verbose, "Task: \"", GetName(), "\" activated.");
            return;
        }

        auto now = Timer::GetCurrentTimeFromEpoch();
        if (now - start > timeout)
        {
            Log(Warning, "Task: \"", GetName(), "\" activation is taking longer (", timeout, "ms) than expected.");
            start = now;
        }

        std::this_thread::yield();
    }
}

void ITask::Deactivate()
{
    auto start = Timer::GetCurrentTimeFromEpoch();
    const auto timeout = 5000;

    while (true)
    {
        State expected = m_State.load(std::memory_order_acquire);
        if (expected == State::Inactive)
        {
            Log(Verbose, "Task: \"", GetName(), "\" has already been deactivated.");
            return;
        }

        if (m_State.compare_exchange_weak(expected, State::Inactive, std::memory_order_acq_rel))
        {
            Log(Verbose, "Task: \"", GetName(), "\" deactivated.");
            return;
        }

        auto now = Timer::GetCurrentTimeFromEpoch();
        if (now - start > timeout)
        {
            Log(Warning, "Task: \"", GetName(), "\" deactivation is taking longer (", timeout, "ms) than expected.");
            start = now;
        }

        std::this_thread::yield();
    }
}

bool ITask::TryToExecute()
{
    State expected = m_State.load(std::memory_order_acquire);
    if (expected == State::Created || expected == State::Inactive || expected == State::Executing)
        return false;

    if (m_State.compare_exchange_strong(expected, State::Executing, std::memory_order_acq_rel))
    {
        if (m_Type == Type::Once)
            Log(Verbose, "Task: \"", GetName(), "\" executing...");

        ExecuteImpl();

        if (m_Type == Type::Once)
        {
            m_State.store(State::Inactive, std::memory_order_release);
            Log(Verbose, "Task: \"", GetName(), "\" finished.");
        }
        else
            m_State.store(State::Done, std::memory_order_release);

        return true;
    }
    
    return false;
}

bool ITask::CanBeRemoved() const
{
    if (m_Type == Type::Once)
        return m_State.load(std::memory_order_acquire) == State::Inactive;
    else
        return false;
}

void ITask::Wait()
{
    auto start = Timer::GetCurrentTimeFromEpoch();
    const auto timeout = 5000;

    while (true)
    {
        State expected = m_State.load(std::memory_order_acquire);
        if (expected == State::Done || expected == State::Inactive)
            return;

        auto now = Timer::GetCurrentTimeFromEpoch();
        if (now - start > timeout)
        {
            Log(Warning, "Task: \"", GetName(), "\" waiting is taking longer (", timeout, "ms) than expected.");
            start = now;
        }

        std::this_thread::yield();
    }
}
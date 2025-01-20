#include "Task.h"

#include "LogService.h"
#include "Timer.h"

#include "../Engine.h"

using namespace Inno;

#include <chrono>

namespace Inno
{
	template <>
	inline void LogService::LogContent(ITask& values)
	{
		LogContent("[Task: ", values.GetName(), " (on thread: ", values.GetThreadIndex(), ")]");
	}

	template <>
	inline void LogService::LogContent(const ITask& values)
	{
		LogContent(const_cast<ITask&>(values));
	}
}

void ITask::Activate()
{
    auto start = Timer::GetCurrentTimeFromEpoch();
    const auto timeout = 5000;

    while (true)
    {
        {
            State expected = m_State.load(std::memory_order_acquire);
            if (expected == State::Waiting || expected == State::Busy)
            {
                Log(Verbose, *this, " has already been activated.");
                return;
            }
        }

        {
            if (m_State.load(std::memory_order_acquire) == State::Released)
            {
                Log(Verbose, *this, " has already been released.");
                return;
            }
        }

        {
            State expected = State::Idle;
            if (m_State.compare_exchange_weak(expected, State::Waiting, std::memory_order_acq_rel))
            {
                if (m_Desc.m_Type == Type::Once)
                    Log(Verbose, *this, " activated for the first time.");
                else
                    Log(Verbose, *this, " activated.");

                m_LastAliveTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
                return;
            }
        }

        auto now = Timer::GetCurrentTimeFromEpoch();
        if (now - start > timeout)
        {
            Log(Warning, *this, " activation is taking longer (", timeout, "ms) than expected.");
            start = now;
        }
    }
}

void ITask::Deactivate()
{
    auto start = Timer::GetCurrentTimeFromEpoch();
    const auto timeout = 5000;

    while (true)
    {
        if (m_State.load(std::memory_order_acquire) == State::Idle)
        {
            Log(Verbose, *this, " has already been deactivated.");
            return;
        }
        
        {
            if (m_State.load(std::memory_order_acquire) == State::Released)
            {
                Log(Verbose, *this, " has already been released.");
                return;
            }
        }

        {
            State expected = State::Waiting;
            if (m_State.compare_exchange_strong(expected, State::Idle, std::memory_order_acq_rel))
            {
                Log(Verbose, *this, " deactivated.");
                return;
            }
        }

        auto now = Timer::GetCurrentTimeFromEpoch();
        if (now - start > timeout)
        {
            Log(Warning, *this, " deactivation is taking longer (", timeout, "ms) than expected.");
            start = now;
        }
    }
}

bool ITask::TryToExecute()
{
    State expected = State::Waiting;
    if (m_State.compare_exchange_strong(expected, State::Busy, std::memory_order_acq_rel))
    {
        if (m_Desc.m_Type == Type::Once)
            Log(Verbose, *this, " executing...");

        ExecuteImpl();
        m_ExecutionCount.fetch_add(1, std::memory_order_acq_rel);

        if (m_Desc.m_Type == Type::Once)
        {
            m_State.store(State::Released, std::memory_order_release);
            Log(Verbose, *this, " finished.");
        }
        else
            m_State.store(State::Waiting, std::memory_order_release);

        {
            auto l_lastAliveTime = m_LastAliveTime;
            m_LastAliveTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

            const auto l_threshold = 1000;
            if (m_Desc.m_Type == Type::Once)
                const auto l_threshold = 5000;

            if (m_LastAliveTime - l_lastAliveTime > l_threshold)
                Log(Warning, *this, " it's been too long (", l_threshold, "ms) since the task was alive last time.");
        }
        
        return true;
    }

    return false;
}

bool ITask::CanBeRemoved() const
{
    return m_State.load(std::memory_order_acquire) == State::Released;
}

void ITask::Wait()
{
    auto start = Timer::GetCurrentTimeFromEpoch();
    const auto timeout = 5000;

    while (true)
    {
        if (m_State.load(std::memory_order_acquire) == State::Idle)
            return;

        if (m_State.load(std::memory_order_acquire) == State::Released)
            return;

        if (m_ExecutionCount.load(std::memory_order_acquire) > 0)
            return;

        auto now = Timer::GetCurrentTimeFromEpoch();
        if (now - start > timeout)
        {
            Log(Warning, *this, " waiting is taking longer (", timeout, "ms) than expected.");
            start = now;
        }

        std::this_thread::yield();
    }
}
#include "Task.h"

#include "LogService.h"

#include "../Engine.h"

using namespace Inno;
;

void ITask::Activate()
{    
    while (true)
    {
        State expected = State::Inactive;

        if (m_State.compare_exchange_weak(expected, State::Waiting, std::memory_order_acq_rel))
        {
            return;
        }

        std::this_thread::yield();
    }
}

void ITask::Deactivate()
{
    while (true)
    {
        State expected = m_State.load(std::memory_order_acquire);

        if (expected == State::Done || expected == State::Waiting || expected == State::Inactive)
        {
            if (m_State.compare_exchange_weak(expected, State::Inactive, std::memory_order_acq_rel))
                return;
        }

        std::this_thread::yield();
    }
}

bool ITask::TryToExecute()
{
    State expected = State::Waiting;
    if (!m_State.compare_exchange_strong(expected, State::Executing, std::memory_order_acq_rel))
    {
        return false;
    }

	Log(Verbose, "Task: \"", GetName(), "\" executing...");
	ExecuteImpl();
    Log(Verbose, "Task: \"", GetName(), "\" finished.");
    
    if (m_Type == Type::Once)
    {
        m_State.store(State::Inactive, std::memory_order_release);
    }
    else
    {
        m_State.store(State::Done, std::memory_order_release);
    }

	return true;
}

void ITask::Wait()
{
    while (true)
    {
        if (m_Type == Type::Once)
        {
            if (m_State.load(std::memory_order_acquire) == State::Inactive)
            {
                return;
            }
        }
        else
        {
            if (m_State.load(std::memory_order_acquire) == State::Done)
            {
                return;
            }
        }

        std::this_thread::yield();
    }
}
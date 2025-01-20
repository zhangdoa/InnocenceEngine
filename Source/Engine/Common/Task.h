#pragma once

#include "STL14.h"
#include "Timer.h"

namespace Inno
{
    class ITask
    {
        friend class TaskScheduler;
    public:
        enum class Type
        {
            Recurrent,
            Once,
        };

        struct Desc
        {
            const char* m_Name;
            Type m_Type;
            uint32_t m_ThreadIndex;

            explicit Desc(const char* name, Type type, uint32_t threadIndex)
                : m_Name(name)
                , m_Type(type)
                , m_ThreadIndex(threadIndex)
            {
            }

            explicit Desc(const char* name, Type type)
                : Desc(name, type, (std::numeric_limits<uint32_t>::max)())
            {
            }

            explicit Desc(const char* name)
                : Desc(name, Type::Recurrent)
            {
            }

            Desc(const Desc& rhs)
                : m_Name(rhs.m_Name)
                , m_Type(rhs.m_Type)
                , m_ThreadIndex(rhs.m_ThreadIndex)
            {
            }
        };

        enum class State
        {
			Idle
			, Waiting
			, Busy
			, Released
        };

        explicit ITask(const Desc& desc)
            : m_Desc(desc)
            , m_LastAliveTime((Timer::GetCurrentTimeFromEpoch(TimeUnit::Millisecond)))
        {
        }

        virtual ~ITask(void) noexcept = default;
        ITask(const ITask& rhs) = delete;
        ITask& operator=(const ITask& rhs) = delete;
        ITask(ITask&& other) = default;
        ITask& operator=(ITask&& other) = default;

        void Activate();
        void Deactivate();   
        bool TryToExecute();
        bool CanBeRemoved() const;
        void Wait();

        virtual void ExecuteImpl() = 0;

        const char* GetName() const
        {
            return m_Desc.m_Name;
        }

        Type GetType() const
        {
            return m_Desc.m_Type;
        }

        const uint32_t& GetThreadIndex() const
        {
            return m_Desc.m_ThreadIndex;
        }

    protected:
        Desc m_Desc;
        std::atomic<State> m_State = State::Idle;
        std::atomic<uint64_t> m_ExecutionCount = 0;
        uint64_t m_LastAliveTime = 0;
    };

    template <typename Functor>
    class Task : public ITask 
    {
    public:
        static_assert(std::is_invocable_r_v<void, Functor>, "Task's Functor must be callable and return void.");

        Task(Functor&& functor, const Desc& desc)
            : ITask(desc), m_Functor(std::forward<Functor>(functor)) {
            }

        ~Task() override {
        }

        Task(const Task& rhs)
            : ITask(rhs.m_Desc), m_Functor(rhs.m_Functor) {
            }

        Task& operator=(const Task& rhs)
        {
            if (this != &rhs)
            {
                ITask::operator=(rhs);
                m_Functor = rhs.m_Functor;
            }

            return *this;
        }

        Task(Task&& other) noexcept
            : ITask(std::move(other)), m_Functor(std::move(other.m_Functor)) {
            }

        Task& operator=(Task&& other) noexcept
        {
            if (this != &other)
            {
                ITask::operator=(std::move(other));
                m_Functor = std::move(other.m_Functor);
            }

            return *this;
        }

    protected:
        void ExecuteImpl() override 
        {
            m_Functor();
        }

    private:
        Functor m_Functor;
    };
}
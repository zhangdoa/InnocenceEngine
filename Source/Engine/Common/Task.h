#pragma once
#include <memory>
#include <type_traits>
#include <future>

#include "STL14.h"

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
            const char* m_Name = "Anonymous Task";
            Type m_Type = Type::Recurrent;
            int32_t m_ThreadID = -1;
        };

        enum class State
        {
            Created,
            Inactive,
            Waiting,
            Done,
            Executing,
        };

        explicit ITask(const char* name, Type type)
            : m_Name(name)
            , m_Type(type)
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
            return m_Name;
        }

        Type GetType() const
        {
            return m_Type;
        }

    protected:
        const char* m_Name = "Anonymous Task";
        Type m_Type = Type::Recurrent;
        std::atomic<State> m_State = State::Created;
    };

    template <typename Functor>
    class Task : public ITask 
    {
    public:
        static_assert(std::is_invocable_r_v<void, Functor>, "Task's Functor must be callable and return void.");

        Task(Functor&& functor, const char* name, Type type)
            : ITask(name, type), m_Functor(std::forward<Functor>(functor)) {
            }

        ~Task() override {
        }

        Task(const Task& rhs)
            : ITask(rhs.m_Name, rhs.m_Type), m_Functor(rhs.m_Functor) {
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
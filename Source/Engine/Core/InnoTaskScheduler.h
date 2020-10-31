#pragma once
#include <memory>
#include <type_traits>
#include <future>
#include "../Common/InnoContainer.h"

namespace Inno
{
	class ITask
	{
	public:
		ITask() = default;
		virtual ~ITask(void) = default;
		ITask(const ITask& rhs) = delete;
		ITask& operator=(const ITask& rhs) = delete;
		ITask(ITask&& other) = default;
		ITask& operator=(ITask&& other) = default;

		virtual void Execute() = 0;
		virtual const char* GetName() = 0;
		virtual const std::shared_ptr<ITask>& GetUpstreamTask() = 0;
		virtual bool IsFinished() = 0;
		virtual void Wait() = 0;
	};

	template <typename Functor>
	class Task : public ITask
	{
	public:
		Task(Functor&& functor, const char* name, const std::shared_ptr<ITask>& upstreamTask)
			:m_Functor{ std::move(functor) }, m_Name{ name }, m_UpstreamTask{ upstreamTask }
		{
		}

		~Task() override = default;
		Task(const Task& rhs) = delete;
		Task& operator=(const Task& rhs) = delete;
		Task(Task&& other) = default;
		Task& operator=(Task&& other) = default;

		void Execute() override
		{
			m_Functor();
			m_IsFinished = true;
		}

		const char* GetName() override
		{
			return m_Name;
		}

		const std::shared_ptr<ITask>& GetUpstreamTask() override
		{
			return m_UpstreamTask;
		}

		bool IsFinished() override
		{
			return m_IsFinished;
		}

		void Wait() override
		{
			while (!m_IsFinished);
		}

	private:
		Functor m_Functor;
		const char* m_Name;
		std::shared_ptr<ITask> m_UpstreamTask;
		std::atomic_bool m_IsFinished = false;
	};

	template <typename T>
	class Future
	{
	public:
		Future(std::future<T>&& future)
			:m_Future{ std::move(future) }
		{
		}

		~Future(void)
		{
			if (m_Future.valid())
			{
				m_Future.get();
			}
		}

		Future(const Future& rhs) = delete;
		Future& operator=(const Future& rhs) = delete;
		Future(Future&& other) = default;
		Future& operator=(Future&& other) = default;

		auto Get(void)
		{
			return m_Future.get();
		}

	private:
		std::future<T> m_Future;
	};

	struct TaskReport
	{
		uint64_t m_StartTime;
		uint64_t m_FinishTime;
		uint32_t m_ThreadID;
		const char* m_TaskName;
	};

	class TaskScheduler
	{
	public:
		static bool Setup();
		static bool Initialize();
		static bool Update();
		static bool Terminate();

		static void WaitSync();

		static std::shared_ptr<ITask> AddTask(std::unique_ptr<ITask>&& task, int32_t threadID);
		static size_t GetThreadCounts();

		static const RingBuffer<TaskReport, true>& GetTaskReport(int32_t threadID);
	};
}
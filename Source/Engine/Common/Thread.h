#pragma once
#include <memory>
#include <type_traits>
#include <future>

#include "ThreadSafeVector.h"
#include "ThreadSafeQueue.h"
#include "RingBuffer.h"
#include "Task.h"

namespace Inno
{
    struct TaskReport
	{
		uint64_t m_StartTime;
		uint64_t m_FinishTime;
		uint32_t m_ThreadID;
		const char* m_TaskName;
	};

    class Thread
	{
	public:
		enum class State
		{
			Idle
			, Waiting
			, Busy
			, Released
		};

		explicit Thread(uint32_t ThreadIndex);

		~Thread(void);

		Thread(const Thread& rhs) = delete;
		Thread& operator=(const Thread& rhs) = delete;

		State GetState() const;
		void Freeze();
		void Unfreeze();

		const RingBuffer<TaskReport, true>& GetTaskReport();

		std::shared_ptr<ITask> AddTask(std::shared_ptr<ITask>&& task);

	private:
		std::string GetThreadID();

		void Worker(uint32_t ThreadIndex);

		bool ExecuteTask(const std::shared_ptr<ITask>& task);

		std::thread* m_ThreadHandle;
		std::pair<uint32_t, std::thread::id> m_ID;
		std::atomic<State> m_State = State::Idle;
		std::atomic_bool m_Done = false;

		ThreadSafeVector<std::shared_ptr<ITask>> m_TaskList;
		ThreadSafeVector<std::shared_ptr<ITask>> m_TaskListToBeRemoved;
		RingBuffer<TaskReport, true> m_TaskReport;
	};
}
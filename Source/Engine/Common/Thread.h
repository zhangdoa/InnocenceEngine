#pragma once
#include <memory>
#include <type_traits>
#include <future>

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
			Initialized
			, Idle
			, Busy
			, Released
		};

		explicit Thread(uint32_t ThreadIndex);

		~Thread(void);

		Thread(const Thread& rhs) = delete;
		Thread& operator=(const Thread& rhs) = delete;

		State GetState() const;

		const RingBuffer<TaskReport, true>& GetTaskReport();

		std::shared_ptr<ITask> AddTask(std::shared_ptr<ITask>&& task);

	private:
		std::string GetThreadID();

		void Worker(uint32_t ThreadIndex);

		void ExecuteTask(std::shared_ptr<ITask>&& task);

		std::thread* m_ThreadHandle;
		std::pair<uint32_t, std::thread::id> m_ID;
		std::atomic<State> m_State = State::Initialized;
		std::atomic_bool m_Done = false;
		ThreadSafeQueue<std::shared_ptr<ITask>> m_WorkQueue;
		RingBuffer<TaskReport, true> m_TaskReport;
	};
}
#pragma once
#include <memory>
#include <type_traits>
#include <future>

#include "Task.h"
#include "SharedPtr.h"
#include "ThreadSafeVector.h"
#include "RingBuffer.h"

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
	    using ID = std::pair<uint32_t, std::thread::id>;

		enum class State
		{
			Idle
			, Waiting
			, Busy
			, Released
			// Worker body itself threw an unhandled exception (not a per-task exception —
			// those are caught and logged inside the Worker loop). Failed threads stop
			// accepting new tasks.
			, Failed
		};

		explicit Thread(uint32_t ThreadIndex);

		~Thread(void);

		Thread(const Thread& rhs) = delete;
		Thread& operator=(const Thread& rhs) = delete;

		State GetState() const;
		void Freeze();
		void Unfreeze();

		const RingBuffer<TaskReport, true>& GetTaskReport();

		void AddTask(SharedPtr<ITask> task);

		// Non-zero means at least one task threw; the thread itself kept running.
		uint64_t GetCaughtExceptionCount() const;

	private:
		void Worker(uint32_t ThreadIndex);

		inline bool ExecuteTask(SharedPtr<ITask> task);

		std::thread* m_ThreadHandle;
		ID m_ID;
		std::atomic<State> m_State = State::Idle;
		std::atomic_bool m_Done = false;
		std::atomic<uint64_t> m_CaughtExceptionCount = 0;

		std::vector<SharedPtr<ITask>> m_TaskList;
		RingBuffer<TaskReport, true> m_TaskReport;
	};
}
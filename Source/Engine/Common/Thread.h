#pragma once
#include <memory>
#include <type_traits>
#include <future>

#include "Task.h"
#include "Handle.h"
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
			// TASK-31: thread terminated due to an unhandled exception escaping the Worker
			// body itself (not a per-task exception — those are caught and logged per task).
			// A Failed thread stops accepting new tasks and is visible through GetState().
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

		void AddTask(Handle<ITask> task);

		// TASK-31: cumulative count of per-task exceptions caught inside the Worker loop.
		// Non-zero means a task threw; the thread itself kept running. Exposed so a
		// TaskScheduler-level health check can trend or alarm on it.
		uint64_t GetCaughtExceptionCount() const;

	private:
		void Worker(uint32_t ThreadIndex);

		inline bool ExecuteTask(Handle<ITask> task);

		std::thread* m_ThreadHandle;
		ID m_ID;
		std::atomic<State> m_State = State::Idle;
		std::atomic_bool m_Done = false;
		std::atomic<uint64_t> m_CaughtExceptionCount = 0;

		std::vector<Handle<ITask>> m_TaskList;
		RingBuffer<TaskReport, true> m_TaskReport;
	};
}
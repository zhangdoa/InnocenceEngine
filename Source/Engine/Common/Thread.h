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

		void AddTask(Handle<ITask> task);

		using ID = std::pair<uint32_t, std::thread::id>;

	private:
		void Worker(uint32_t ThreadIndex);

		inline bool ExecuteTask(Handle<ITask> task);

		std::thread* m_ThreadHandle;
		ID m_ID;
		std::atomic<State> m_State = State::Idle;
		std::atomic_bool m_Done = false;

		std::vector<Handle<ITask>> m_TaskList;
		RingBuffer<TaskReport, true> m_TaskReport;
	};
}
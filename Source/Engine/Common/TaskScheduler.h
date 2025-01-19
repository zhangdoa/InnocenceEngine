#pragma once
#include <memory>
#include <type_traits>
#include <future>

#include "STL14.h"
#include "Thread.h"
#include "Task.h"
#include "Handle.h"
#include "RingBuffer.h"

namespace Inno
{
	class TaskScheduler
	{
	public:
		TaskScheduler();
        ~TaskScheduler();

        void Reset();

		void Freeze();
		void Unfreeze();
		
		template <typename Func, typename... Args>
		auto Submit(const ITask::Desc& taskDesc, Func&& func, Args&&... args) 
		{
			auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
			auto task = new Task<decltype(boundTask)>
			(
				std::move(boundTask), taskDesc.m_Name, taskDesc.m_Type
			);

			auto handle = Handle<ITask>(task);
			
			AddTask(handle, taskDesc.m_ThreadID);
			return handle;
		}

		template <typename Func, typename... Args>
		auto Submit(const char* name, int32_t threadID, Func&& func, Args&&... args)
		{
			ITask::Desc taskDesc;
			taskDesc.m_Name = name;
			taskDesc.m_ThreadID = threadID;

			return Submit(taskDesc, std::forward<Func>(func), std::forward<Args>(args)...);
		}

		void AddTask(Handle<ITask> task, int32_t threadID);
		bool AddDependency(Handle<ITask> task, Handle<ITask> dependency);
		size_t GetThreadCounts();

		const RingBuffer<TaskReport, true>& GetTaskReport(int32_t threadID);

	private:
		std::atomic_size_t m_NumThreads = 0;
		std::vector<std::unique_ptr<Thread>> m_Threads;
	};
}
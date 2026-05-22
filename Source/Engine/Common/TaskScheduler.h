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
			auto l_threadIndex = GenerateThreadIndex(taskDesc.m_ThreadIndex);
			auto l_taskDesc =  ITask::Desc(taskDesc.m_Name, taskDesc.m_Type, l_threadIndex);
			auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
			auto task = new Task<decltype(boundTask)>
			(
				std::move(boundTask), l_taskDesc
			);

			auto handle = Handle<ITask>(task);
			
			AddTask(handle, l_threadIndex);
			return handle;
		}

		bool AddDependency(Handle<ITask> task, Handle<ITask> dependency);
		size_t GetThreadCounts();

		const RingBuffer<TaskReport, true>& GetTaskReport(uint32_t threadIndex);

		void AddTask(Handle<ITask> task, uint32_t threadIndex);
		
	private:
		uint32_t GenerateThreadIndex(uint32_t threadIndex);

		std::atomic_size_t m_NumThreads = 0;
		std::vector<std::unique_ptr<Thread>> m_Threads;
	};
}
#pragma once
#include <memory>
#include <type_traits>
#include <future>

#include "STL14.h"
#include "Thread.h"
#include "Task.h"
#include "RingBuffer.h"

namespace Inno
{
	class TaskScheduler
	{
	public:
		TaskScheduler();
		~TaskScheduler();

		void Freeze();
		void Unfreeze();
		
		template <typename Func, typename... Args>
		auto Submit(const ITask::Desc& taskDesc, Func&& func, Args&&... args) 
		{
			auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
			auto task = std::make_unique<Task<decltype(boundTask)>>
			(
				std::move(boundTask), taskDesc.m_Name, taskDesc.m_Type
			);

			return AddTask(std::move(task), taskDesc.m_ThreadID);
		}

		template <typename Func, typename... Args>
		auto Submit(const char* name, int32_t threadID, Func&& func, Args&&... args)
		{
			ITask::Desc taskDesc;
			taskDesc.m_Name = name;
			taskDesc.m_ThreadID = threadID;

			return Submit(taskDesc, std::forward<Func>(func), std::forward<Args>(args)...);
		}

		std::shared_ptr<ITask> AddTask(std::unique_ptr<ITask>&& task, int32_t threadID);
		bool AddDependency(std::shared_ptr<ITask>&& task, std::shared_ptr<ITask>&& dependency);
		size_t GetThreadCounts();

		const RingBuffer<TaskReport, true>& GetTaskReport(int32_t threadID);

	private:
		std::atomic_size_t m_NumThreads = 0;
		std::vector<std::unique_ptr<Thread>> m_Threads;
	};
}
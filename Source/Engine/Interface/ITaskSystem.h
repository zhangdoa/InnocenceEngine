#pragma once
#include "ISystem.h"
#include "../Core/TaskScheduler.h"

namespace Inno
{
	template <typename T>
	struct TaskHandle
	{
		std::shared_ptr<ITask> m_Task;
		std::shared_ptr<Future<T>> m_Future;
	};

	class ITaskSystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ITaskSystem);

		virtual void WaitSync() = 0;

		virtual const RingBuffer<TaskReport, true>& GetTaskReport(int32_t threadID) = 0;
		virtual size_t GetThreadCounts() = 0;

		template <typename Func, typename... Args>
		auto Submit(const char* name, int32_t threadID, const std::shared_ptr<ITask>& upstreamTask, Func&& func, Args&&... args)
		{
			auto l_BoundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
			using ResultType = std::invoke_result_t<decltype(l_BoundTask)>;
			using PackagedTask = std::packaged_task<ResultType()>;

			PackagedTask l_Task{ std::move(l_BoundTask) };
			Future<ResultType> l_Future{ l_Task.get_future() };
			TaskHandle<ResultType> l_Handle;
			l_Handle.m_Task = AddTask(std::make_unique<Task<PackagedTask>>(std::move(l_Task), name, upstreamTask), threadID);
			l_Handle.m_Future = std::make_shared<Future<ResultType>>(std::move(l_Future));

			return l_Handle;
		}

	protected:
		virtual std::shared_ptr<ITask> AddTask(std::unique_ptr<ITask>&& task, int32_t threadID) = 0;
	};
}

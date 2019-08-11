#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "InnoTaskScheduler.h"

class ITaskSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITaskSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual void waitAllTasksToFinish() = 0;

	virtual const RingBuffer<InnoTaskReport, true>& GetTaskReport(int threadID) = 0;
	virtual size_t GetTotalThreadsNumber() = 0;

	template <typename Func, typename... Args>
	std::shared_ptr<IInnoTask> submit(const char* name, int threadID, const std::shared_ptr<IInnoTask>& upstreamTask, Func&& func, Args&&... args)
	{
		auto BoundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
		using ResultType = std::invoke_result_t<decltype(BoundTask)>;
		using PackagedTask = std::packaged_task<ResultType()>;
		using TaskType = InnoTask<PackagedTask>;

		PackagedTask Task{ std::move(BoundTask) };
		return addTaskImpl(std::make_unique<TaskType>(std::move(Task), name, upstreamTask), threadID);
	}

protected:
	virtual std::shared_ptr<IInnoTask> addTaskImpl(std::unique_ptr<IInnoTask>&& task, int threadID) = 0;
};

#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoClassTemplate.h"
#include "InnoTaskScheduler.h"

INNO_INTERFACE ITaskSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITaskSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual void waitAllTasksToFinish() = 0;

	virtual std::string getThreadId() = 0;

	template <typename Func, typename... Args>
	IInnoTask* submit(const char* name, Func&& func, Args&&... args, int threadID = -1)
	{
		auto BoundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
		using ResultType = std::invoke_result_t<decltype(BoundTask)>;
		using PackagedTask = std::packaged_task<ResultType()>;
		using TaskType = InnoTask<PackagedTask>;

		PackagedTask Task{ std::move(BoundTask) };
		return addTaskImpl(std::make_unique<TaskType>(std::move(Task), name), threadID);
	}

protected:
	virtual IInnoTask* addTaskImpl(std::unique_ptr<IInnoTask>&& task, int threadID) = 0;
};

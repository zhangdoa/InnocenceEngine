#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"
#include "../common/InnoConcurrency.h"

INNO_INTERFACE ITaskSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITaskSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual void* addTask(std::unique_ptr<IThreadTask>&& task) = 0;

	virtual void shrinkFutureContainer(std::vector<InnoFuture<void>>& rhs) = 0;

	virtual void waitAllTasksToFinish() = 0;

	virtual std::string getThreadId() = 0;

	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
		using ResultType = std::invoke_result_t<decltype(boundTask)>;
		using PackagedTask = std::packaged_task<ResultType()>;
		using TaskType = InnoTask<PackagedTask>;

		PackagedTask task{ std::move(boundTask) };
		return addTask(std::make_unique<TaskType>(std::move(task)));;
	}
};

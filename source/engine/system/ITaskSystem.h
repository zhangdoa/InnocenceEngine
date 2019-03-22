#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/InnoConcurrency.h"

INNO_INTERFACE ITaskSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITaskSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual void addTask(std::unique_ptr<IThreadTask>&& task) = 0;

	INNO_SYSTEM_EXPORT virtual void shrinkFutureContainer(std::vector<InnoFuture<void>>& rhs) = 0;

	INNO_SYSTEM_EXPORT virtual void waitAllTasksToFinish() = 0;

	INNO_SYSTEM_EXPORT virtual std::string getThreadId() = 0;

	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
		using ResultType = std::invoke_result_t<decltype(boundTask)>;
		using PackagedTask = std::packaged_task<ResultType()>;
		using TaskType = InnoTask<PackagedTask>;

		PackagedTask task{ std::move(boundTask) };
		InnoFuture<ResultType> result{ task.get_future() };
		addTask(std::make_unique<TaskType>(std::move(task)));
		return result;
	}
};

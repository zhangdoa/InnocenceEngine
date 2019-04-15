#pragma once
#include "../ITaskSystem.h"

class InnoTaskSystem : INNO_IMPLEMENT ITaskSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTaskSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus()  override;

	void* addTask(std::unique_ptr<IThreadTask>&& task) override;

	void shrinkFutureContainer(std::vector<InnoFuture<void>>& rhs) override;

	void waitAllTasksToFinish() override;

	std::string getThreadId() override;
};

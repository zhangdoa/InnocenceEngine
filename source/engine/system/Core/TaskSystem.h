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

	void waitAllTasksToFinish() override;

	std::string getThreadId() override;

protected:
	IInnoTask* addTaskImpl(std::unique_ptr<IInnoTask>&& task, int threadID) override;
};

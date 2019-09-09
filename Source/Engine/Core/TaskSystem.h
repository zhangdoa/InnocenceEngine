#pragma once
#include "ITaskSystem.h"

class InnoTaskSystem : public ITaskSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTaskSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus()  override;

	void waitAllTasksToFinish() override;

	const RingBuffer<InnoTaskReport, true>& GetTaskReport(int32_t threadID) override;
	size_t GetTotalThreadsNumber() override;

protected:
	std::shared_ptr<IInnoTask> addTaskImpl(std::unique_ptr<IInnoTask>&& task, int32_t threadID) override;
};

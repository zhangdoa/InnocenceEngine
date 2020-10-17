#pragma once
#include "../Interface/ITaskSystem.h"

namespace Inno
{
	class InnoTaskSystem : public ITaskSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTaskSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus()  override;

		void waitAllTasksToFinish() override;

		const RingBuffer<InnoTaskReport, true>& GetTaskReport(int32_t threadID) override;
		size_t GetTotalThreadsNumber() override;

	protected:
		std::shared_ptr<IInnoTask> addTaskImpl(std::unique_ptr<IInnoTask>&& task, int32_t threadID) override;
	};
}

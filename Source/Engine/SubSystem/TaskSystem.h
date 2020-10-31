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

		void WaitSync() override;

		const RingBuffer<TaskReport, true>& GetTaskReport(int32_t threadID) override;
		size_t GetThreadCounts() override;

	protected:
		std::shared_ptr<ITask> AddTask(std::unique_ptr<ITask>&& task, int32_t threadID) override;
	};
}

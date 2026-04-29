#pragma once
#include "../Interface/IService.h"
#include "../Component/GPUBufferComponent.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct PerFrameDataServiceImpl;
	class PerFrameDataService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PerFrameDataService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const PerFrameConstantBuffer& GetPerFrameConstantBuffer();
		GPUBufferComponent* GetCurrentFrameBuffer();
		GPUBufferComponent* GetPreviousFrameBuffer();

		// TASK-183 runtime debug-view mode. Set from any thread (DevToggleRegistry
		// callbacks fire on the editor IPC thread); read on the render thread
		// during UpdatePerFrameConstantBuffer. Atomic to keep the cross-thread
		// write tear-free without taking the impl mutex.
		void SetDebugViewMode(DebugViewMode in_Mode);
		DebugViewMode GetDebugViewMode() const;

	private:
		PerFrameDataServiceImpl* m_Impl;
	};
}

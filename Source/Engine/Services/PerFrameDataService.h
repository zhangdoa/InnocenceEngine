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

		// Setters callable from any thread; getters snapshot on the render thread.
		// Atomic-backed so the cross-thread write is tear-free without the impl mutex.
		void SetDebugViewMode(DebugViewMode in_Mode);
		DebugViewMode GetDebugViewMode() const;

		void SetPointShadowBypass(bool in_Bypass);
		bool GetPointShadowBypass() const;

	private:
		PerFrameDataServiceImpl* m_Impl;
	};
}

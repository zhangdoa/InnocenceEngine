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

		// TASK-195 runtime point-shadow bypass (A/B toggle for inline-RT shadow
		// trace in lightPassDirectLighting.hlsl::EvaluateTiledPointLighting).
		// Same threading shape as the debug-view mode above — atomic bool set
		// on the editor IPC thread, snapshotted into PerFrame_CB on the render
		// thread. Replaces the former compile-time #define DEBUG_POINT_SHADOW_BYPASS.
		void SetPointShadowBypass(bool in_Bypass);
		bool GetPointShadowBypass() const;

	private:
		PerFrameDataServiceImpl* m_Impl;
	};
}

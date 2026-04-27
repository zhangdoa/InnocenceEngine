#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class TextureComponent;

	// TASK-138 phase 1 — hardware-RT sun-shadow visibility producer.
	//
	// Per-pixel R8 visibility texture. One thread per screen pixel traces a
	// single cone-jittered shadow ray toward the sun (~0.5° half-angle from
	// SUN_ANGULAR_RADIUS in common.hlsl). Output: 0.0 = fully shadowed,
	// 1.0 = lit. TAA accumulation across frames softens the penumbra.
	//
	// Phase 1 ships ADDITIVE: this pass produces a texture that LightPass can
	// optionally consume, but CSM+PCSS (SunShadowGeometryProcessPass +
	// SunShadowResolver) stays the default. The cost decision (swap, keep
	// both with gate, or hold off) is driven by post-CL PIX measurement —
	// see TASK-138 task description and `.alignments/TASK-138-rt-sun-shadows-design.md`.
	class SunShadowRTPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SunShadowRTPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		// Per-pixel sun visibility. R8 single-channel float. Consumed by
		// LightPass at slot t13 (additive — co-exists with the CSM atlas at
		// t7 until the swap decision is made).
		TextureComponent* GetResult();

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		RenderPassComponent* m_RenderPassComp = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;

		TextureComponent* m_SunVisibility = nullptr;

		ShaderStage m_ShaderStage = ShaderStage::Invalid;

		bool RenderTargetsCreationFunc();
		void OnResize();
		void CreateVisibilityBuffer();
	};
} // namespace Inno

#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class TextureComponent;

	// TASK-138 — hardware-RT sun-shadow visibility producer (sole sun-shadow
	// path after the CSM+PCSS swap landed).
	//
	// Per-pixel R8 visibility texture. One thread per screen pixel traces a
	// single cone-jittered shadow ray toward the sun (~0.5° half-angle from
	// SUN_ANGULAR_RADIUS in common.hlsl). Output: 0.0 = fully shadowed,
	// 1.0 = lit. TAA accumulation across frames softens the penumbra.
	//
	// LightPass binds this at slot t13 and consumes it in
	// lightPassDirectLighting.hlsl::EvaluateSunLighting. See TASK-138
	// description + `.alignments/TASK-138-rt-sun-shadows-design.md`.
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
		// LightPass at slot t13.
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

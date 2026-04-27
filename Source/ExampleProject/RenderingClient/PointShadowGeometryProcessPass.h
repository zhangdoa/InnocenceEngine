#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// Per-frame cube-shadow caster. Walks the LightDataService-allocated atlas
	// (Texture2DArray, DepthOrArraySize = maxPointShadows * 6) and for every
	// active shadow-casting point/sphere light writes packed depth (depth, depth²)
	// to slices [base..base+5] via geometry-shader instanced cube-face fan-out.
	//
	// Mirrors SunShadowGeometryProcessPass (CSM packed-depth) — the resolver
	// (shadowResolver.hlsl::PointShadowResolver) reads .r as blocker depth and
	// .g for the variance/depth² channel using the same convention as
	// SunShadowResolver (returned shadow ∈ [0,1] where 1 = fully shadowed).
	class PointShadowGeometryProcessPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PointShadowGeometryProcessPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		uint32_t GetShadowMapResolution();
		// The atlas TextureComponent is owned by LightDataService; this getter
		// proxies through so LightPass / the resolver bind the same resource the
		// caster wrote.
		GPUResourceComponent* GetResult();

	private:
		// Reservation hook: wires the externally-allocated atlas (owned by
		// LightDataService) into m_OutputMergerTarget->m_ColorOutputs[0]. The
		// renderpass service's default reservation would Add() a new
		// TextureComponent for our RT, which would shadow the slot allocator's
		// resource and break LightPass's SRV binding. AnimationPass.cpp uses
		// the same idiom to alias OpaquePass's RTs.
		bool RenderTargetsReservationFunc();

		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		SamplerComponent* m_SamplerComp;

		uint32_t m_PerFaceResolution = 256;
	};
} // namespace Inno

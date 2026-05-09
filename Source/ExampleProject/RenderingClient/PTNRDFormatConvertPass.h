#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"

namespace Inno
{
	// Engine PT-output -> NVIDIA NRD ReBLUR input format-conversion compute
	// pass (TASK-77.4 CL-2). Pure data-shaping: reads the path-tracer's
	// primary-hit GBuffer (RT0..RT3) and the per-lobe radiance UAVs (raygen
	// outputs at u11/u12), packs them through NRD's front-end helpers, and
	// writes the five textures the ReBLUR_DiffuseSpecular denoiser consumes
	// (IN_VIEWZ, IN_NORMAL_ROUGHNESS, IN_MV, IN_DIFF_RADIANCE_HITDIST,
	// IN_SPEC_RADIANCE_HITDIST).
	//
	// HLSL shader: Source/Shaders/HLSL/PTNRDFormatConvert.comp.
	//
	// CL-2 ships the format-convert outputs to UAVs that no later pass reads;
	// CL-3 will wire NRD's denoise dispatch + composition. With CL-2 alone
	// the displayed output reverts to the baseline 1-spp PT path (this pass
	// dispatches but its outputs are unconsumed) — that is the documented
	// CL-2 end-state per the task plan.
	//
	// Toggle gate: keyed on Inno::NRD::ENABLED (CMake BUILD_WITH_NRD). With
	// the toggle off, Setup keeps the pass Terminated and no resource is
	// allocated — bypass invariant matches the cache / PT-denoise pattern.
	// PTDenoise::ENABLED gates the upstream UAV writes; this pass requires
	// both toggles to fire usefully, but only NRD::ENABLED gates allocation:
	// when PTDenoise::ENABLED is false the upstream textures are nullptr and
	// PrepareCommandList early-outs.
	//
	// Owned resources (5 RWTexture2D outputs at screen resolution):
	//   - m_NRD_ViewZ                R32F      (IN_VIEWZ)
	//   - m_NRD_NormalRoughness      RGBA8     (IN_NORMAL_ROUGHNESS surrogate
	//                                            — NRD wants R10G10B10A2_UNORM
	//                                            but the engine format mapper
	//                                            lacks that DXGI tag; CL-3
	//                                            will either widen the engine
	//                                            format enum or flip
	//                                            NRDConfig's normal-encoding.)
	//   - m_NRD_MotionVector         RG16F     (IN_MV, 2D-pixel mode)
	//   - m_NRD_DiffRadianceHitDist  RGBA16F   (IN_DIFF_RADIANCE_HITDIST)
	//   - m_NRD_SpecRadianceHitDist  RGBA16F   (IN_SPEC_RADIANCE_HITDIST)
	//
	// Schedule: dispatched on the Compute queue every frame after
	// GPUPathTracerPass finishes (the path tracer writes the input UAVs and
	// transitions them to ReadOnly at the end of its dispatch). Same shape
	// as the cache passes — graphics CL pre-pass for state transitions on
	// the output UAVs, compute CL for the kernel.
	class PTNRDFormatConvertPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTNRDFormatConvertPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		// NRD-format outputs. Returned pointers are owned by this pass; CL-3
		// will borrow them as inputs to PTNRDDenoisePass. nullptr when
		// Inno::NRD::ENABLED is false.
		TextureComponent* GetNRDViewZ()                { return m_NRD_ViewZ; }
		TextureComponent* GetNRDNormalRoughness()      { return m_NRD_NormalRoughness; }
		TextureComponent* GetNRDMotionVector()         { return m_NRD_MotionVector; }
		TextureComponent* GetNRDDiffRadianceHitDist()  { return m_NRD_DiffRadianceHitDist; }
		TextureComponent* GetNRDSpecRadianceHitDist()  { return m_NRD_SpecRadianceHitDist; }

	private:
		ObjectStatus            m_ObjectStatus      = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;

		// Single-buffered NRD-input outputs. Allocated in Initialize on the
		// resolution the engine reports at that moment; recreated by
		// OnResize when the swap chain resizes (NRD's prev-frame
		// reconstruction handles the resolution change at its own layer
		// once CL-3 lands the denoise dispatch).
		TextureComponent* m_NRD_ViewZ                = nullptr;
		TextureComponent* m_NRD_NormalRoughness      = nullptr;
		TextureComponent* m_NRD_MotionVector         = nullptr;
		TextureComponent* m_NRD_DiffRadianceHitDist  = nullptr;
		TextureComponent* m_NRD_SpecRadianceHitDist  = nullptr;

		// Texture-creation helper. Called by Initialize (allocation site) and
		// by m_RenderPassComp->m_OnResize (re-allocation after a swap-chain
		// resize). Mirrors the GPUPathTracerPass::CreatePTGBufferTextures
		// pattern — the format-convert outputs are pure scratch and any
		// resolution change scraps them outright.
		bool RenderTargetsCreationFunc();
	};
} // namespace Inno

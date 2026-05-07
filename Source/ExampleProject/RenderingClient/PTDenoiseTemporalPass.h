#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"

namespace Inno
{
	// Screen-space PT denoiser temporal accumulator (TASK-77.2 CL-2).
	// Per-pixel motion-vector reprojection + per-lobe history blending
	// against the SVGF-shape disocclusion gates (mesh-id strict equality,
	// relative depth tolerance, normal-dot threshold; constants in
	// common/PTDenoiseShared.hlsl). Produces the per-lobe (radiance,
	// sampleCount) + (Σ luma, Σ luma²) history textures that CL-3's
	// à-trous spatial filter will read.
	//
	// CL-2 ships invisibly behind the AccumBuffer write — the path
	// tracer integrator still feeds tonemap with the summed lobes; this
	// pass populates the history textures in parallel. CL-3/CL-4 wire
	// the history into the displayed output.
	//
	// Owned resources:
	//   - Per-lobe current-frame radiance UAVs (RGBA16F, single-buffer).
	//     Borrowed back by GPUPathTracerPass during dispatch — raygen
	//     writes radianceDiffuse / radianceSpecular into them at the
	//     AccumBuffer composition site.
	//   - Per-lobe history textures (RGBA16F radiance + RG16F moments,
	//     ping-pong on FrameCountSinceLaunch). 4 textures × 2 frames
	//     per lobe × 2 lobes = 8 textures.
	//
	// Reference: SVGF (Schied et al. 2017) §3-§4.2.
	//
	// Toggle gate: when PTDenoise::ENABLED is false, Setup keeps the
	// pass Terminated, no resource allocation, no dispatch — bypass
	// invariant holds bit-identical to the toggle-OFF path tracer.
	class PTDenoiseTemporalPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTDenoiseTemporalPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		// Per-lobe current-frame radiance — written by GPUPathTracerRayGen
		// and consumed once per frame by this pass. Single-buffered.
		// nullptr when PTDenoise::ENABLED is false.
		TextureComponent* GetCurrentRadianceDiffuse()  { return m_RadianceDiffuse; }
		TextureComponent* GetCurrentRadianceSpecular() { return m_RadianceSpecular; }

		// Ping-pong history accessors. The "current" set is the write
		// target this frame (and the next-frame Prev source); the
		// "previous" set holds last frame's blend. Parity follows
		// FrameCountSinceLaunch.
		TextureComponent* GetCurrentHistoryRadianceDiffuse();
		TextureComponent* GetCurrentHistoryMomentsDiffuse();
		TextureComponent* GetCurrentHistoryRadianceSpecular();
		TextureComponent* GetCurrentHistoryMomentsSpecular();
		TextureComponent* GetPreviousHistoryRadianceDiffuse();
		TextureComponent* GetPreviousHistoryMomentsDiffuse();
		TextureComponent* GetPreviousHistoryRadianceSpecular();
		TextureComponent* GetPreviousHistoryMomentsSpecular();

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;

		// Per-lobe current-frame radiance — single-buffered RGBA16F.
		TextureComponent* m_RadianceDiffuse  = nullptr;
		TextureComponent* m_RadianceSpecular = nullptr;

		// Per-lobe history radiance (RGBA16F: rgb=blended radiance,
		// a=sample count clamped at PT_DENOISE_MAX_HISTORY_FRAMES) and
		// moments (RG16F: r=Σ luma/N, g=Σ luma²/N). Variance recovered
		// at read time as `max(g - r*r, 0)`. Ping-pong by FrameCount.
		TextureComponent* m_HistoryRadianceDiffuse_Even   = nullptr;
		TextureComponent* m_HistoryRadianceDiffuse_Odd    = nullptr;
		TextureComponent* m_HistoryMomentsDiffuse_Even    = nullptr;
		TextureComponent* m_HistoryMomentsDiffuse_Odd     = nullptr;
		TextureComponent* m_HistoryRadianceSpecular_Even  = nullptr;
		TextureComponent* m_HistoryRadianceSpecular_Odd   = nullptr;
		TextureComponent* m_HistoryMomentsSpecular_Even   = nullptr;
		TextureComponent* m_HistoryMomentsSpecular_Odd    = nullptr;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno

#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// PT hash-grid cache running-mean resolve. Consumes the per-cell scratch
	// sums atomically accumulated by GPUPathTracerRayGen on the previous
	// frame's bounce loop, blends them into the persistent ValueBuffer with
	// a 16-sample-cap running-mean update rule, and zeroes the scratch so
	// the next frame's writes accumulate fresh. Mirrors the MIP-0 block of
	// Capsaicin GI-1.0 UpdateTiles (gi1.comp:2160-2225 — direct lobe;
	// gi1.comp:2183-2223 — indirect-lobe twin under USE_MULTI_BOUNCE);
	// the mip-cascade lives in PTHashGridCacheMipCascadeBuildPass.
	//
	// D1-reversal CL B: this pass now resolves both pairs in lockstep —
	// direct (UpdateCellValueBuffer → ValueBuffer, capped at MAX_SAMPLE_COUNT)
	// and indirect (UpdateCellValueIndirectBuffer → ValueIndirectBuffer,
	// capped at MAX_MULTIBOUNCE_SAMPLE_COUNT). Until CL C wires the
	// integrator's secondary-bounce write into the indirect scratch, the
	// indirect resolve runs against zero data and is a runtime no-op;
	// binding count is 1 CB + 5 UAVs (was 1 CB + 3 UAVs).
	//
	// Only meaningful when PTHashGridCache::ENABLED is true. With the
	// toggle off, Setup / Initialize / PrepareCommandList all return true
	// without touching any GPU resource and ExampleRenderingClient skips
	// dispatch — the pass is structurally inert under the bypass invariant.
	//
	// Schedule: dispatched on the Compute queue every frame, ordered before
	// GPUPathTracerPass so the path tracer reads ValueBuffer with the
	// most-recent resolved running mean and writes into the now-empty
	// scratch. The cache UAVs themselves live on GPUPathTracerPass; this
	// pass borrows them by name through the singleton, no buffer ownership
	// crosses the boundary.
	class PTHashGridCacheUpdateTilesPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTHashGridCacheUpdateTilesPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;
	};
} // namespace Inno

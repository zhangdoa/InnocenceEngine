#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// PT hash-grid cache mip-cascade build. After PTHashGridCacheUpdateTiles
	// resolves the per-cell scratch sums into ValueBuffer / ValueIndirectBuffer
	// at mip 0, this pass aggregates 2x2 children into one parent at each
	// level: mip 0 -> mip 1, mip 1 -> mip 2, mip 2 -> mip 3. The aggregation
	// rule is a 4-way sum of the radiance * sample_count packed cells
	// (Capsaicin GI-1.0 gi1.comp:2227-2347 — the mip-1 / mip-2 / mip-3
	// blocks of UpdateTiles fused with groupshared LDS for the direct lobe;
	// gi1.comp:2252-2345 — the `if (USE_MULTI_BOUNCE)` indirect-lobe twin);
	// since storage is already weighted by sample count, summing four
	// children produces a valid coarser cell without renormalisation.
	// Mirrors hash_grid_cache.hlsl:207-221 cell-index hashing across mip
	// levels.
	//
	// D1-reversal CL B: this pass now builds the cascade for both the
	// direct ValueBuffer and the indirect ValueIndirectBuffer in lockstep.
	// Until CL C lights up the indirect-scratch writers, the indirect
	// mip-0 reads come back as zero so the cascade aggregation produces
	// zero parents; runtime no-op, structural delta only. Binding count
	// is 1 CB + 3 UAVs (was 1 CB + 2 UAVs).
	//
	// Capsaicin fuses the cascade into UpdateTiles in a single dispatch with
	// a 2D 8x8 thread group; we run a separate pass so the UpdateTiles pass
	// can stay a focused mip-0 resolve. Documented divergence — pipeline
	// shape only, the aggregation maths are paper-faithful.
	//
	// Only meaningful when PTHashGridCache::ENABLED is true. With the toggle
	// off, Setup / Initialize / PrepareCommandList all return true without
	// touching any GPU resource and ExampleRenderingClient skips dispatch —
	// the pass is structurally inert under the bypass invariant. The cascade
	// contents are written but not yet read this CL: mip-aware lookup at the
	// Site-3 read in GPUPathTracerRayGen lands in the next CL, so the
	// cascade is dead data here. The structural correctness gate (build
	// fires, no D3D12 errors, no perturbation of the toggle-on visual at
	// f30) is what closes this CL.
	//
	// Schedule. Compute queue. Ordered after PurgeTiles + UpdateTiles, before
	// the path tracer. Same-queue Signal/Wait pairs match the existing cache
	// passes; no graphics-side fence. The cache UAVs themselves live on
	// GPUPathTracerPass; this pass borrows HashBuffer + ValueBuffer + the
	// HashGridCache CB by accessor and never owns them.
	class PTHashGridCacheMipCascadeBuildPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTHashGridCacheMipCascadeBuildPass)

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

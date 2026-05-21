#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// PT hash-grid cache mip-cascade build. After PTHashGridCacheUpdateTiles
	// resolves the per-cell scratch sums into ValueBuffer / ValueIndirectBuffer
	// at mip 0, this pass aggregates 2x2 children into one parent at each
	// level for both lobes in lockstep: mip 0 -> mip 1, mip 1 -> mip 2,
	// mip 2 -> mip 3. The aggregation rule is a 4-way sum of the
	// radiance * sample_count packed cells (Capsaicin GI-1.0
	// gi1.comp:2227-2347 — direct cascade; gi1.comp:2252-2345 — indirect
	// `USE_MULTI_BOUNCE` twin); since storage is already weighted by sample
	// count, summing four children produces a valid coarser cell without
	// renormalisation. Mirrors hash_grid_cache.hlsl:207-221 cell-index
	// hashing across mip levels.
	//
	// Binding count: 1 CB + 3 UAVs (HashBuffer + ValueBuffer + ValueIndirectBuffer).
	//
	// Capsaicin fuses the cascade into UpdateTiles in a single dispatch with
	// a 2D 8x8 thread group; we run a separate pass so the UpdateTiles pass
	// can stay a focused mip-0 resolve. Documented divergence — pipeline
	// shape only, the aggregation maths are paper-faithful.
	//
	// Only meaningful when PTHashGridCache::ENABLED is true. With the toggle
	// off, Setup / Initialize / PrepareCommandList all return true without
	// touching any GPU resource and ExampleRenderingClient skips dispatch —
	// the pass is structurally inert under the bypass invariant. The
	// path-tracer Site-3 read currently consumes mip 0 only; mip 1-3
	// contents are reserved for downstream consumers (debug visualisation
	// or a future mip-aware read).
	//
	// Schedule. Compute queue. Ordered after PurgeTiles + UpdateTiles, before
	// the path tracer. Same-queue Signal/Wait pairs match the existing cache
	// passes; no graphics-side fence. The cache UAVs themselves live on
	// PTPass; this pass borrows HashBuffer + ValueBuffer + the
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

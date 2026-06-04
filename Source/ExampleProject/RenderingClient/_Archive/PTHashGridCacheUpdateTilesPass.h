#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// PT hash-grid cache running-mean resolve. Consumes the per-cell scratch
	// sums atomically accumulated by PTRayGen on the previous
	// frame's bounce loop, blends them into the persistent ValueBuffer /
	// ValueIndirectBuffer with sample-count-capped running-mean updates, and
	// zeroes the scratch so the next frame's writes accumulate fresh.
	// Mirrors the MIP-0 block of Capsaicin GI-1.0 UpdateTiles
	// (gi1.comp:2160-2180 direct lobe; gi1.comp:2183-2223 indirect twin
	// under USE_MULTI_BOUNCE). Direct lobe caps at MAX_SAMPLE_COUNT (16);
	// indirect lobe caps at MAX_MULTIBOUNCE_SAMPLE_COUNT (16) — held as
	// independent knobs (Capsaicin gi1.h:63 vs gi1.h:65). The mip-cascade
	// lives in PTHashGridCacheMipCascadeBuildPass.
	//
	// Binding count: 1 CB + 5 UAVs (HashBuffer + direct pair + indirect pair).
	//
	// Only meaningful when PTHashGridCache::ENABLED is true. With the
	// toggle off, Setup / Initialize / PrepareCommandList all return true
	// without touching any GPU resource and ExampleRenderingClient skips
	// dispatch — the pass is structurally inert under the bypass invariant.
	//
	// Schedule: dispatched on the Compute queue every frame, ordered before
	// PTPass so the path tracer reads ValueBuffer with the
	// most-recent resolved running mean and writes into the now-empty
	// scratch. The cache UAVs themselves live on PTPass; this
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

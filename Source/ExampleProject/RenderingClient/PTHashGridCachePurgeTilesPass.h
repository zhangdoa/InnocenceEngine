#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// PT hash-grid cache LRU eviction. Walks every tile slot once per frame
	// and frees any whose DecayTileBuffer marker has fallen more than
	// TILE_DECAY_FRAMES (50, matching Capsaicin gi1_shared.h
	// kHashGridCache_TileDecay) behind the current frame counter. A freed
	// slot has its HashBuffer entry zeroed so the next InsertCell that
	// hashes into the same bucket can re-claim it via
	// InterlockedCompareExchange. Mirrors Capsaicin GI-1.0 PurgeTiles
	// (gi1.comp:1715-1752); the dirty-tile re-pack into PackedTileIndexBuffer
	// is intentionally not ported because the engine does not run an
	// indirect-dispatch UpdateTiles.
	//
	// Only meaningful when PTHashGridCache::ENABLED is true. With the
	// toggle off, Setup / Initialize / PrepareCommandList all return true
	// without touching any GPU resource and ExampleRenderingClient skips
	// dispatch — the pass is structurally inert under the bypass invariant.
	//
	// Schedule: dispatched on the Compute queue every frame, ordered before
	// PTHashGridCacheUpdateTilesPass. The order PurgeTiles → UpdateTiles →
	// GPUPathTracer matches Capsaicin's pipeline: PurgeTiles frees stale
	// slots first so UpdateTiles' HashBuffer == 0 early-out skips them
	// (avoiding a running-mean merge against a doomed tile) and the path
	// tracer's InsertCell can claim the freed slot the same frame. The
	// cache UAVs themselves live on GPUPathTracerPass; this pass borrows
	// HashBuffer + DecayTileBuffer + the FrameCount CB by accessor and
	// never owns them.
	class PTHashGridCachePurgeTilesPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTHashGridCachePurgeTilesPass)

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

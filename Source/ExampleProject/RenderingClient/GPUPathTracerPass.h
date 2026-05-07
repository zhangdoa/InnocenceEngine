#pragma once
#include <unordered_set>
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"
#include "../../Engine/Component/GPUBufferComponent.h"
#include "../../Engine/Component/SamplerComponent.h"
#include "../../Engine/Common/EntityID.h"
#include "../../Engine/Common/GPUDataStructure.h"

namespace Inno
{
	class GPUPathTracerPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GPUPathTracerPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		GPUResourceComponent* GetResult();


		void ResetAccumulation();

		// Hash-grid radiance cache buffer accessors. Returned pointers are
		// owned by GPUPathTracerPass; PTHashGridCacheUpdateTilesPass borrows
		// them on the same Compute queue per frame and never deletes them.
		// When PTHashGridCache::ENABLED is false these always return nullptr.
		GPUBufferComponent* GetHashGridCacheCB()                  { return m_HashGridCacheCB; }
		GPUBufferComponent* GetHashGridCacheHashBuffer()          { return m_HashGridCache_HashBuffer; }
		GPUBufferComponent* GetHashGridCacheDecayTileBuffer()     { return m_HashGridCache_DecayTileBuffer; }
		GPUBufferComponent* GetHashGridCacheUpdateCellValueBuffer() { return m_HashGridCache_UpdateCellValueBuffer; }
		GPUBufferComponent* GetHashGridCacheValueBuffer()         { return m_HashGridCache_ValueBuffer; }
		// Indirect-mirror pair (Capsaicin gi1.cpp:497-553 — separate
		// UpdateCellValueIndirectBuffer / ValueIndirectBuffer alongside the
		// direct pair under the `gi1_use_multibounce` branch). Carries the
		// secondary-bounce contribution as its own running-mean estimator;
		// the Site-3 read sums per-lobe means before substitution.
		GPUBufferComponent* GetHashGridCacheUpdateCellValueIndirectBuffer() { return m_HashGridCache_UpdateCellValueIndirectBuffer; }
		GPUBufferComponent* GetHashGridCacheValueIndirectBuffer() { return m_HashGridCache_ValueIndirectBuffer; }
		// FrameCount CB exposed so PTHashGridCachePurgeTilesPass can compute
		// frame_count - decay marker without owning a parallel CB upload.
		GPUBufferComponent* GetFrameCountCB()                     { return m_FrameCountCB; }

	private:
		struct PathTracerLightCountData
		{
			uint32_t pointLightCount;
			uint32_t sphereLightCount;
			uint32_t pad0;
			uint32_t pad1;
		};

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		// DXR ray tracing pass
		RenderPassComponent*    m_RayTracingRenderPassComp = nullptr;
		ShaderProgramComponent* m_RayTracingSPC            = nullptr;

		// Owned GPU resources
		TextureComponent*   m_AccumulationBuffer = nullptr;
		GPUBufferComponent* m_FrameCountCB       = nullptr;
		GPUBufferComponent* m_LightCountCB       = nullptr;
		SamplerComponent*   m_MaterialSampler    = nullptr;

		// PT hash-grid radiance cache resources (Capsaicin GI-1.0
		// hash_grid_cache.hlsl port). All nullptr unless
		// PTHashGridCache::ENABLED is true and Setup/Initialize ran. When
		// disabled, no allocation, no binding, no dispatch-side cost — the
		// cache-off bypass invariant.
		GPUBufferComponent* m_HashGridCacheCB                  = nullptr;
		GPUBufferComponent* m_HashGridCache_HashBuffer         = nullptr;
		GPUBufferComponent* m_HashGridCache_DecayTileBuffer    = nullptr;
		GPUBufferComponent* m_HashGridCache_UpdateCellValueBuffer = nullptr;
		GPUBufferComponent* m_HashGridCache_ValueBuffer        = nullptr;
		// Indirect-mirror pair (Capsaicin gi1.cpp:497-553 multibounce
		// branch). uint[4] per cell for the atomic scratch, uint2 per cell
		// for the persistent estimator. Cleared on scene load alongside
		// the direct pair.
		GPUBufferComponent* m_HashGridCache_UpdateCellValueIndirectBuffer = nullptr;
		GPUBufferComponent* m_HashGridCache_ValueIndirectBuffer = nullptr;

		GPUBufferComponent* m_MaterialBuffer = nullptr;

		Math::Mat4 m_PrevViewMatrix = {};
		uint32_t m_FrameCount     = 1;

		std::function<void()> f_sceneLoadedCallback;
		std::function<void()> f_sceneUnloadingCallback;

		ShaderStage m_ShaderStage = ShaderStage::Invalid;
		bool m_PendingMaterialRebuild = false;
		bool m_HashGridCachePendingClear = false;
		size_t m_BuiltMeshCount = 0;

		std::vector<MaterialConstantBuffer> m_PendingMaterials;

		std::unordered_set<EntityID> m_WarnedMissingMaterial;

		void RebuildMaterialBuffer();
		void RefreshMaterialTextureIndices();
		void CreateAccumulationBuffer();
		void OnResize();
	};
}

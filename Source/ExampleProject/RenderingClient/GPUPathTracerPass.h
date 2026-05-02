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
		// FrameCount CB exposed so PTHashGridCachePurgeTilesPass can compute
		// frame_count - decay marker without owning a parallel CB upload.
		GPUBufferComponent* GetFrameCountCB()                     { return m_FrameCountCB; }

	private:
		struct GPUPathTracerVertex
		{
			float posX, posY, posZ;
			float normX, normY, normZ;
			float texU, texV;
		};

		struct MeshOffsetData
		{
			uint32_t m_VertexOffset;
			uint32_t m_IndexOffset;
			uint32_t m_VertexCount;
			uint32_t m_IndexCount;
		};

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

		// Geometry mega-buffers (rebuilt on scene load)
		GPUBufferComponent* m_MegaVertexBuffer = nullptr;
		GPUBufferComponent* m_MegaIndexBuffer  = nullptr;
		GPUBufferComponent* m_MeshOffsetBuffer = nullptr;
		GPUBufferComponent* m_MaterialBuffer   = nullptr;

		// Camera movement detection
		Math::Mat4 m_PrevViewMatrix = {};
		uint32_t m_FrameCount     = 1;

		// Scene callbacks
		std::function<void()> f_sceneLoadedCallback;
		std::function<void()> f_sceneUnloadingCallback;

		ShaderStage m_ShaderStage = ShaderStage::Invalid;
		bool m_PendingGeometryRebuild = false;
		bool m_HashGridCachePendingClear = false;
		size_t m_BuiltMeshCount = 0;

		// Persistent storage for deferred GPU upload (m_InitialData points here)
		std::vector<GPUPathTracerVertex>    m_PendingVertices;
		std::vector<uint32_t>               m_PendingIndices;
		std::vector<MeshOffsetData>         m_PendingOffsets;
		std::vector<MaterialConstantBuffer> m_PendingMaterials;

		// Entities already warned about missing/unresolvable materials, so
		// RebuildGeometryBuffers doesn't spam the log once per rebuild.
		std::unordered_set<EntityID> m_WarnedMissingMaterial;

		void RebuildGeometryBuffers();
		void RefreshMaterialTextureIndices();
		bool AreMeshesGPUReady();
		void CreateAccumulationBuffer();
		void OnResize();
	};
}

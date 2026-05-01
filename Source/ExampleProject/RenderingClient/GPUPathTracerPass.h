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

		// Hash-grid + per-pixel hit data exposed for downstream consumers
		// (TASK-77.1.2 GPUPathTracerDenoisePass). The hash-grid pair is the
		// world-space radiance cache; the per-pixel hit pair lets a screen-
		// space consumer turn a pixel back into the (posWS, N) lookup key
		// the writer used at primary hit. Both pairs are persistently
		// Accessibility::ReadWrite UAVs in this pass — downstream passes
		// rebind them as ReadOnly inside their own command lists.
		GPUBufferComponent* GetHashGridKeys();
		GPUBufferComponent* GetHashGridCells();
		TextureComponent*   GetPrimaryHitPosBuffer();
		TextureComponent*   GetPrimaryHitNormalBuffer();

		void ResetAccumulation();

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

		// World-space hash-grid radiance cache (TASK-77.1 phase 1).
		// Owned by this pass — the only writer is the PT raygen at primary
		// hit, and downstream consumers (TASK-77.1.2 denoise pass) read the
		// same buffers via the engine's resource registry.
		GPUBufferComponent* m_HashGridKeys  = nullptr;
		GPUBufferComponent* m_HashGridCells = nullptr;

		// Per-pixel primary-hit position + normal (TASK-77.1.2). Written by
		// the PT raygen alongside the noisy buffer so the denoise pass can
		// reconstruct the writer's hash-grid key at the same screen pixel
		// without re-tracing primary rays or borrowing the rasterizer's
		// G-buffer (PT-primary's load-bearing invariant). Both screen-res
		// RGBA Float32; .w = 1.0 marks a valid hit, .w = 0.0 marks miss /
		// sky so the consumer can short-circuit lookups.
		TextureComponent*   m_PrimaryHitPosBuffer    = nullptr;
		TextureComponent*   m_PrimaryHitNormalBuffer = nullptr;

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
		void CreatePrimaryHitBuffers();
		void OnResize();
	};
}

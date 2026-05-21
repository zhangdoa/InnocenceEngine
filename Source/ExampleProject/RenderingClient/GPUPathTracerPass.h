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

		// Hash-grid cache buffers are GPUPathTracerPass-owned; borrowed by
		// PTHashGridCacheUpdateTilesPass on the same Compute queue, never deleted.
		// All nullptr unless PTHashGridCache::ENABLED.
		GPUBufferComponent* GetHashGridCacheCB()                  { return m_HashGridCacheCB; }
		GPUBufferComponent* GetHashGridCacheHashBuffer()          { return m_HashGridCache_HashBuffer; }
		GPUBufferComponent* GetHashGridCacheDecayTileBuffer()     { return m_HashGridCache_DecayTileBuffer; }
		GPUBufferComponent* GetHashGridCacheUpdateCellValueBuffer() { return m_HashGridCache_UpdateCellValueBuffer; }
		GPUBufferComponent* GetHashGridCacheValueBuffer()         { return m_HashGridCache_ValueBuffer; }
		GPUBufferComponent* GetHashGridCacheUpdateCellValueIndirectBuffer() { return m_HashGridCache_UpdateCellValueIndirectBuffer; }
		GPUBufferComponent* GetHashGridCacheValueIndirectBuffer() { return m_HashGridCache_ValueIndirectBuffer; }
		GPUBufferComponent* GetFrameCountCB()                     { return m_FrameCountCB; }

		// All nullptr unless PTDenoise::ENABLED. NRD ReBLUR reconstructs prev-frame internally
		// via motion vectors, so these are single-buffered.
		TextureComponent* GetPTGBufferPosition()        { return m_PTGBuffer_Position; }
		TextureComponent* GetPTGBufferNormalMetalness() { return m_PTGBuffer_NormalMetalness; }
		TextureComponent* GetPTGBufferAlbedoRoughness() { return m_PTGBuffer_AlbedoRoughness; }
		TextureComponent* GetPTGBufferMotionHitDist()   { return m_PTGBuffer_MotionHitDist; }
		TextureComponent* GetPTRadianceDiffuse()        { return m_PTRadianceDiffuse; }
		TextureComponent* GetPTRadianceSpecular()       { return m_PTRadianceSpecular; }

	private:
		struct PathTracerLightCountData
		{
			uint32_t pointLightCount;
			uint32_t sphereLightCount;
			uint32_t pad0;
			uint32_t pad1;
		};

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		RenderPassComponent*    m_RayTracingRenderPassComp = nullptr;
		ShaderProgramComponent* m_RayTracingSPC            = nullptr;

		TextureComponent*   m_AccumulationBuffer = nullptr;
		GPUBufferComponent* m_FrameCountCB       = nullptr;
		GPUBufferComponent* m_LightCountCB       = nullptr;
		SamplerComponent*   m_MaterialSampler    = nullptr;

		// All nullptr unless PTHashGridCache::ENABLED. Cache-off → no allocation, no binding,
		// no dispatch-side cost.
		GPUBufferComponent* m_HashGridCacheCB                  = nullptr;
		GPUBufferComponent* m_HashGridCache_HashBuffer         = nullptr;
		GPUBufferComponent* m_HashGridCache_DecayTileBuffer    = nullptr;
		GPUBufferComponent* m_HashGridCache_UpdateCellValueBuffer = nullptr;
		GPUBufferComponent* m_HashGridCache_ValueBuffer        = nullptr;
		GPUBufferComponent* m_HashGridCache_UpdateCellValueIndirectBuffer = nullptr;
		GPUBufferComponent* m_HashGridCache_ValueIndirectBuffer = nullptr;

		// All nullptr unless PTDenoise::ENABLED. Layout mirrors opaqueGeometryProcessPass.frag so
		// DecodeGBuffer in common/lightPassCommon.hlsl reads them unchanged.
		TextureComponent* m_PTGBuffer_Position        = nullptr; // RT0: positionWS + instanceID
		TextureComponent* m_PTGBuffer_NormalMetalness = nullptr; // RT1: normalWS  + metalness
		TextureComponent* m_PTGBuffer_AlbedoRoughness = nullptr; // RT2: albedo    + roughness
		TextureComponent* m_PTGBuffer_MotionHitDist   = nullptr; // RT3: motionVec + hitDist + 0
		TextureComponent* m_PTRadianceDiffuse         = nullptr; // raygen u11: per-lobe diffuse radiance
		TextureComponent* m_PTRadianceSpecular        = nullptr; // raygen u12: per-lobe specular radiance

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
		void CreatePTGBufferTextures();
		void DeletePTGBufferTextures();
		void OnResize();

		void ConfigureRaytracingBindings();
	};
}

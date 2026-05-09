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

		// Screen-space PT denoiser GBuffer-equivalent textures (TASK-77.2
		// CL-1; ping-pong dropped in TASK-77.4 CL-2). Channel layout per
		// common/PTDenoiseShared.hlsl, mirroring opaqueGeometryProcessPass.frag
		// so DecodeGBuffer reads them unchanged. Single-buffered: NRD ReBLUR
		// owns prev-frame reconstruction via motion vectors, so the engine
		// never needs to read last frame's GBuffer-equivalent textures.
		// Per-lobe radiance UAVs (CL-2 raygen output) and the four GBuffer
		// channels are all single-buffered with this same accessor shape.
		// nullptr when PTDenoise::ENABLED is false.
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

		// Screen-space PT denoiser GBuffer-equivalent textures (TASK-77.2
		// CL-1 introduced; ping-pong dropped in TASK-77.4 CL-2 because NRD
		// ReBLUR reconstructs prev-frame internally from motion vectors).
		// Written by GPUPathTracerRayGen.hlsl at bounce == 0 under
		// PT_DENOISE_ENABLED; consumed by PTNRDFormatConvertPass on the same
		// frame. Layout mirrors opaqueGeometryProcessPass.frag so
		// DecodeGBuffer in common/lightPassCommon.hlsl reads them unchanged.
		// Per-lobe radiance UAVs travel alongside on the same toggle —
		// raygen writes them at the AccumBuffer composition site, the
		// format-convert pass reads them, NRD denoises them in CL-3.
		// All nullptr unless PTDenoise::ENABLED is true and Setup/Initialize
		// ran. Bypass invariant: when disabled, no allocation, no binding,
		// no shader bytes emitted, AccumBuffer write is bit-identical.
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

		// Raytracing-pass binding-layout descriptor table population. Lives in
		// GPUPathTracerPass_BindingLayout.cpp so the layout-cluster (12 base
		// + 7 cache + 5 denoise descriptor entries with their static_assert
		// invariants) does not push Setup.cpp past the file-size ratchet.
		// Same TU-class as Setup; called once from Setup() after the
		// RenderPassComponent is created and before the descriptor vector
		// is consumed by Initialize.
		void ConfigureRaytracingBindings();
	};
}

#include "GPUPathTracerPass.h"
#include "HashGridCacheConstants.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/LightDataService.h"
#include "../../Engine/Component/MeshComponent.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::Update()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	size_t l_currentMeshOwnerCount = l_meshStorage.AllOwners().size();

	if (l_currentMeshOwnerCount != m_BuiltMeshCount)
		m_PendingMaterialRebuild = true;

	if (m_PendingMaterialRebuild)
	{
		RebuildMaterialBuffer();
		m_PendingMaterialRebuild = false;
	}

	// Per-frame re-resolve of material texture indices. Texture activation
	// can lag the scene-load callback by multiple frames (TASK-74 — Sponza
	// curtain BaseColor textures arrived after the initial material rebuild),
	// so each frame any newly-Activated texture gets its bindless index
	// folded back into the material buffer.
	RefreshMaterialTextureIndices();

	const bool l_ready = m_MaterialBuffer && m_MaterialBuffer->m_ObjectStatus == ObjectStatus::Activated;
	m_ObjectStatus = l_ready ? ObjectStatus::Activated : ObjectStatus::Suspended;

	if (!l_ready)
		return true;

	const auto& l_perFrameCB = g_Engine->Get<PerFrameDataService>()->GetPerFrameConstantBuffer();

	if (std::memcmp(&l_perFrameCB.v, &m_PrevViewMatrix, sizeof(Math::Mat4)) != 0)
	{
		m_FrameCount = 1;
		m_PrevViewMatrix = l_perFrameCB.v;
	}
	else
	{
		m_FrameCount++;
	}

	if (m_FrameCountCB && m_FrameCountCB->m_ObjectStatus == ObjectStatus::Activated)
	{
		g_Engine->Get<GPUBufferResourceService>()->Upload(m_FrameCountCB, &m_FrameCount);
	}

	if (m_LightCountCB && m_LightCountCB->m_ObjectStatus == ObjectStatus::Activated)
	{
		auto l_lightService = g_Engine->Get<LightDataService>();
		PathTracerLightCountData l_lightCounts;
		l_lightCounts.pointLightCount  = l_lightService->GetPointLightCount();
		l_lightCounts.sphereLightCount = l_lightService->GetSphereLightCount();
		l_lightCounts.pad0 = 0;
		l_lightCounts.pad1 = 0;
		g_Engine->Get<GPUBufferResourceService>()->Upload(m_LightCountCB, &l_lightCounts);
	}

	if constexpr (Inno::PTHashGridCache::ENABLED)
	{
		if (m_HashGridCacheCB && m_HashGridCacheCB->m_ObjectStatus == ObjectStatus::Activated)
		{
			using namespace Inno::PTHashGridCache;

			// Angular-pixel footprint per unit distance, recomputed each frame so
			// the cell-size formula tracks the current FOV and viewport. Mirrors
			// Capsaicin gi1.cpp:1856-1859: tan(fovY * knob * max(1/h, h/(w*w))).
			// fovY recovered from the projection matrix: p_original[1][1] =
			// 1/tan(fovY/2) (engine's row-major Vulkan-style perspective).
			const auto& l_perFrameCB = g_Engine->Get<PerFrameDataService>()->GetPerFrameConstantBuffer();
			auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
			const float l_pyy = l_perFrameCB.p_original.m11;
			const float l_fovY = l_pyy > 0.0f ? 2.0f * std::atan(1.0f / l_pyy) : 1.047f; // ~60° fallback
			const float l_w = static_cast<float>(l_resolution.x > 0u ? l_resolution.x : 1u);
			const float l_h = static_cast<float>(l_resolution.y > 0u ? l_resolution.y : 1u);
			const float l_pixelFactor = std::max(1.0f / l_h, l_h / (l_w * l_w));
			const float l_angular = std::tan(l_fovY * CELL_SIZE_KNOB * l_pixelFactor);

			HashGridCacheConstants l_cacheConsts = {};
			l_cacheConsts.num_buckets                  = NUM_BUCKETS;
			l_cacheConsts.num_tiles_per_bucket         = NUM_TILES_PER_BUCKET;
			l_cacheConsts.tile_cell_ratio              = TILE_CELL_RATIO;
			l_cacheConsts.num_cells_per_tile           = NUM_CELLS_PER_TILE;
			l_cacheConsts.size_tile_mip0               = SIZE_TILE_MIP0;
			l_cacheConsts.size_tile_mip1               = SIZE_TILE_MIP1;
			l_cacheConsts.size_tile_mip2               = SIZE_TILE_MIP2;
			l_cacheConsts.size_tile_mip3               = SIZE_TILE_MIP3;
			l_cacheConsts.first_cell_offset_tile_mip0  = FIRST_CELL_OFFSET_TILE_MIP0;
			l_cacheConsts.first_cell_offset_tile_mip1  = FIRST_CELL_OFFSET_TILE_MIP1;
			l_cacheConsts.first_cell_offset_tile_mip2  = FIRST_CELL_OFFSET_TILE_MIP2;
			l_cacheConsts.first_cell_offset_tile_mip3  = FIRST_CELL_OFFSET_TILE_MIP3;
			l_cacheConsts.cell_size                    = l_angular;
			l_cacheConsts.min_cell_size                = MIN_CELL_SIZE;
			l_cacheConsts.max_sample_count             = MAX_SAMPLE_COUNT;
			l_cacheConsts.pad0                         = 0.0f;
			g_Engine->Get<GPUBufferResourceService>()->Upload(m_HashGridCacheCB, &l_cacheConsts);
		}
	}

	return true;
}

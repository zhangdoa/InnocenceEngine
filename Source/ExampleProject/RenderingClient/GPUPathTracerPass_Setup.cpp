#include "GPUPathTracerPass.h"
#include "HashGridCacheConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::Setup(IServiceConfig* systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderStage = ShaderStage::RayGen | ShaderStage::ClosestHit | ShaderStage::AnyHit | ShaderStage::Miss;

	// --- Ray Tracing SPC ---
	m_RayTracingSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("GPUPathTracerPass");
	m_RayTracingSPC->m_ShaderFilePaths.m_RayGenPath     = "GPUPathTracerRayGen.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_ClosestHitPath = "GPUPathTracerClosestHit.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_AnyHitPath     = "GPUPathTracerAnyHit.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_MissPath       = "GPUPathTracerMiss.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_ShadowMissPath = "GPUPathTracerShadowMiss.hlsl";

	// --- Ray Tracing Render Pass ---
	m_RayTracingRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("GPUPathTracerPass");

	auto l_rtDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_rtDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_rtDesc.m_RenderTargetCount = 0;
	l_rtDesc.m_UseRaytracing    = true;
	l_rtDesc.m_UseOutputMerger  = false;

	m_RayTracingRenderPassComp->m_RenderPassDesc = l_rtDesc;

	// Owned accumulation UAV isn't an output-merger target so the frame-
	// management resize path would otherwise skip it. Hook OnResize so the
	// buffer is recreated at the new resolution and accumulation history
	// is scrapped.
	m_RayTracingRenderPassComp->m_OnResize = [this]() { OnResize(); };

	constexpr size_t l_baseBindingCount  = 12;
	constexpr size_t l_cacheBindingCount = Inno::PTHashGridCache::ENABLED ? 7 : 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs.resize(l_baseBindingCount + l_cacheBindingCount);

	// b0 - PerFrameCB (set 0, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex   = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage       = m_ShaderStage;

	// b1 - FrameCountCB (set 0, binding 1)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex   = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage       = m_ShaderStage;

	// t0 - TLAS (set 1, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUBufferUsage         = GPUBufferUsage::TLAS;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage            = m_ShaderStage;

	// t1 - MaterialBuffer (set 1, binding 1, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex        = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage            = m_ShaderStage;

	// t8 - bindless per-mesh vertex SRV array
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUBufferUsage         = GPUBufferUsage::BindlessMeshVertex;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex        = 8;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage            = m_ShaderStage;

	// t9 - bindless per-mesh index SRV array
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUBufferUsage         = GPUBufferUsage::BindlessMeshIndex;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex        = 9;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage            = m_ShaderStage;

	// u0 - AccumulationBuffer (set 2, binding 0, ReadWrite UAV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType        = GPUResourceType::Image;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex      = 2;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage           = TextureUsage::ComputeOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage            = m_ShaderStage;

	// b2 - LightCountCB (set 0, binding 2)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex   = 2;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage       = m_ShaderStage;

	// t5 - PointLightBuffer (set 1, binding 5, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex        = 5;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_ShaderStage            = m_ShaderStage;

	// t6 - SphereLightBuffer (set 1, binding 6, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex        = 6;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_ShaderStage            = m_ShaderStage;

	// t7 - bindless material textures (set 1, binding 7, unbounded Texture2D array).
	// Engine auto-populates this descriptor table from the read-only texture heap;
	// the closest-hit shader indexes it with MaterialCB::TextureIndices (same index
	// space as the rasterizer's OpaquePass at register t3).
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType        = GPUResourceType::Image;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_TextureUsage           = TextureUsage::Sample;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex        = 7;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_ShaderStage            = m_ShaderStage;

	// s0 - material sampler (set 3, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType        = GPUResourceType::Sampler;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex      = 3;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_ShaderStage            = m_ShaderStage;

	if constexpr (Inno::PTHashGridCache::ENABLED)
	{
		// b3 - HashGridCacheCB (set 0, binding 3)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_GPUResourceType   = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorSetIndex = 0;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorIndex   = 3;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_ShaderStage       = m_ShaderStage;

		// u1 - HashBuffer (set 2, binding 1, ReadWrite UAV)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorIndex        = 1;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_ShaderStage            = m_ShaderStage;

		// u2 - DecayTileBuffer (set 2, binding 2, ReadWrite UAV)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorIndex        = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_ShaderStage            = m_ShaderStage;

		// u3 - UpdateCellValueBuffer (set 2, binding 3, ReadWrite UAV)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorIndex        = 3;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_ShaderStage            = m_ShaderStage;

		// u4 - ValueBuffer (set 2, binding 4, ReadWrite UAV) — Site-3 read
		// target. Sources its values from PTHashGridCacheUpdateTilesPass'
		// running-mean resolve of UpdateCellValueBuffer.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorIndex        = 4;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_ShaderStage            = m_ShaderStage;

		// u5 - UpdateCellValueIndirectBuffer (set 2, binding 5, ReadWrite UAV).
		// D1-reversal CL C — the (b) secondary-bounce write target. Capsaicin
		// gi1.comp:1948-1989 UpdateMultibounceCells writes the BRDF/pdf-
		// modulated tertiary-cell mean into the previous vertex's *indirect*
		// scratch; the integrator's (b) write redirects here from
		// UpdateCellValueBuffer this CL.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorIndex        = 5;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_ShaderStage            = m_ShaderStage;

		// u6 - ValueIndirectBuffer (set 2, binding 6, ReadWrite UAV) —
		// reserved for the indirect-lobe Site-3 read in CL D. Slot is bound
		// this CL so the root signature width is stable across CL C and CL D
		// (the b9a103cc PSO-failure precedent — descriptor-table growth is
		// the load-bearing failure surface). No HLSL read references this
		// CL; only the binding declaration in GPUPathTracerRayGen.hlsl.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorIndex        = 6;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_ShaderStage            = m_ShaderStage;
	}

	m_MaterialSampler = g_Engine->Get<SamplerResourceService>()->Add("GPUPathTracerMaterialSampler");
	m_MaterialSampler->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_MaterialSampler->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_RayTracingRenderPassComp->m_ShaderProgram = m_RayTracingSPC;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	// --- Scene callbacks ---
	f_sceneLoadedCallback = [this]()
	{
		m_PendingMaterialRebuild = true;
		m_FrameCount = 1;
		m_HashGridCachePendingClear = true;
	};

	f_sceneUnloadingCallback = [this]()
	{
		auto l_bufService = g_Engine->Get<GPUBufferResourceService>();
		if (m_MaterialBuffer)
		{
			l_bufService->Delete(m_MaterialBuffer);
			m_MaterialBuffer = nullptr;
		}
		m_BuiltMeshCount = 0;
		m_FrameCount = 1;
		m_ObjectStatus = ObjectStatus::Suspended;
	};

	g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadedCallback);
	g_Engine->Get<SceneService>()->AddSceneUnloadingCallback(&f_sceneUnloadingCallback);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

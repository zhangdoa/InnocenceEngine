#include "GPUPathTracerPass.h"
#include "HashGridCacheConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/LightDataService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RayTracingRenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_MaterialBuffer || m_MaterialBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_perFrameBuffer  = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_resolution      = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	// Graphics CL: transition textures to compute-writable states.
	// Must happen on Graphics because tracked state may include PIXEL_SHADER_RESOURCE
	// (set by swap chain presentation), which is invalid on compute command lists.
	l_fmService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Graphics);

	// Compute CL: bind and dispatch rays
	l_fmService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_perFrameBuffer,                 0);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_FrameCountCB,                   1);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, g_Engine->Get<GPUBufferResourceService>()->GetTLASBuffer(), 2);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MaterialBuffer,                   3);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, nullptr,                           4);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, nullptr,                           5);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_AccumulationBuffer,              6);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_LightCountCB,                    7);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, g_Engine->Get<LightDataService>()->GetPointLightBuffer(),  8);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, g_Engine->Get<LightDataService>()->GetSphereLightBuffer(), 9);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, nullptr,                                                  10);
	l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MaterialSampler,                                        11);

	if constexpr (Inno::PTHashGridCache::ENABLED)
	{
		auto l_bufService = g_Engine->Get<GPUBufferResourceService>();

		if (m_HashGridCachePendingClear)
		{
			l_bufService->Clear(m_CommandListComp_Compute, m_HashGridCache_HashBuffer);
			l_bufService->Clear(m_CommandListComp_Compute, m_HashGridCache_DecayTileBuffer);
			l_bufService->Clear(m_CommandListComp_Compute, m_HashGridCache_UpdateCellValueBuffer);
			l_bufService->Clear(m_CommandListComp_Compute, m_HashGridCache_ValueBuffer);
			l_bufService->Clear(m_CommandListComp_Compute, m_HashGridCache_UpdateCellValueIndirectBuffer);
			l_bufService->Clear(m_CommandListComp_Compute, m_HashGridCache_ValueIndirectBuffer);
			m_HashGridCachePendingClear = false;
		}

		l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_HashGridCacheCB,                             12);
		l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_HashGridCache_HashBuffer,                    13);
		l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_HashGridCache_DecayTileBuffer,               14);
		l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_HashGridCache_UpdateCellValueBuffer,         15);
		l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_HashGridCache_ValueBuffer,                   16);
		l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_HashGridCache_UpdateCellValueIndirectBuffer, 17);
		l_fmService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_HashGridCache_ValueIndirectBuffer,           18);
	}

	l_fmService->DispatchRays(m_RayTracingRenderPassComp, m_CommandListComp_Compute, l_resolution.x, l_resolution.y, 1);
	l_fmService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_fmService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

	return true;
}

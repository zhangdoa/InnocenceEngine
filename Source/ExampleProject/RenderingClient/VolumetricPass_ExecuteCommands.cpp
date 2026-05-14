#include "VolumetricPass_Internal.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/LightDataService.h"
#include "../../Engine/Services/DrawCallService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool VolumetricPass::froxelization()
{

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_MeshGPUBufferComp = g_Engine->Get<DrawCallService>()->GetGPUModelDataBuffer();
	auto l_MaterialGPUBufferComp = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();

	// l_fmService->CommandListBegin(m_froxelizationCommandListComp, m_froxelizationRenderPassComp, 0);
	// l_fmService->BindRenderPassComponent(m_froxelizationRenderPassComp, m_froxelizationCommandListComp);
	// l_fmService->ClearRenderTargets(m_froxelizationRenderPassComp, m_froxelizationCommandListComp);

	// l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[0], 3);
	// l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[1], 4);

	// for (uint32_t i = 0; i < l_drawCallCount; i++)
	// {
	// 	auto l_drawCallData = l_drawCallInfo[i];
	// 	auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::MainCamera);
	// 	if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
	// 	{
	// 		if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
	// 		{
	// 			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
	// 			{
	// 				l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.m_PerObjectConstantBufferIndex, 1);
	// 				l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.m_PerObjectConstantBufferIndex, 1);

	// 				l_fmService->DrawIndexedInstanced(m_froxelizationRenderPassComp, l_drawCallData.mesh);
	// 			}
	// 		}
	// 	}
	// }

	// l_fmService->CommandListEnd(m_froxelizationRenderPassComp, m_froxelizationCommandListComp);

	return true;
}

bool VolumetricPass::irraidanceInjection()
{

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_PointLightGPUBufferComp = g_Engine->Get<LightDataService>()->GetPointLightBuffer();
	// TODO: Implement per-pass dispatch params buffer for VolumetricPass

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = m_voxelizationResolution.z;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = (uint32_t)std::ceil((float)l_numThreadsZ / 8.0f);

	DispatchParamsConstantBuffer l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// TODO: Implement per-pass dispatch params buffer upload
	// g_Engine->Get<GPUBufferResourceService>()->Upload(l_dispatchParamsGPUBufferComp, &l_irraidanceInjectionWorkload, 6, 1);

	// l_fmService->CommandListBegin(m_irraidanceInjectionCommandListComp, m_irraidanceInjectionRenderPassComp, 0);
	// l_fmService->BindRenderPassComponent(m_irraidanceInjectionRenderPassComp, m_irraidanceInjectionCommandListComp);
	// l_fmService->ClearRenderTargets(m_irraidanceInjectionRenderPassComp, m_irraidanceInjectionCommandListComp);

	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_SamplerComp, 9);

	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 2);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 3);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0], 4);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, SunShadowRTPass::Get().GetResult(), 5);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 6);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 7);

	// l_fmService->Dispatch(m_irraidanceInjectionRenderPassComp, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	// l_fmService->CommandListEnd(m_irraidanceInjectionRenderPassComp, m_irraidanceInjectionCommandListComp);

	return true;
}

bool VolumetricPass::rayMarching()
{
	GPUResourceComponent* l_currentResultBinder;
	GPUResourceComponent* l_historyResultBinder;

	if (m_isPassA)
	{
		l_currentResultBinder = m_rayMarchingResult_A;
		l_historyResultBinder = m_rayMarchingResult_B;
		m_isPassA = false;
	}
	else
	{
		l_currentResultBinder = m_rayMarchingResult_B;
		l_historyResultBinder = m_rayMarchingResult_A;
		m_isPassA = true;
	}

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	// TODO: Implement per-pass dispatch params buffer for VolumetricPass

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = 1;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = 1;

	DispatchParamsConstantBuffer l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// TODO: Implement per-pass dispatch params buffer upload
	// g_Engine->Get<GPUBufferResourceService>()->Upload(l_dispatchParamsGPUBufferComp, &l_rayMarchingWorkload, 7, 1);

	// l_fmService->CommandListBegin(m_rayMarchingCommandListComp, m_rayMarchingRenderPassComp, 0);
	// l_fmService->BindRenderPassComponent(m_rayMarchingRenderPassComp, m_rayMarchingCommandListComp);
	// l_fmService->ClearRenderTargets(m_rayMarchingRenderPassComp, m_rayMarchingCommandListComp);

	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_SamplerComp, 7);

	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 2);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0], 3);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[1], 4);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_historyResultBinder, 5);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_currentResultBinder, 6);

	// l_fmService->Dispatch(m_rayMarchingRenderPassComp, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	// l_fmService->CommandListEnd(m_rayMarchingRenderPassComp, m_rayMarchingCommandListComp);

	return true;
}

bool VolumetricPass::visualization(GPUResourceComponent* input)
{
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_MeshGPUBufferComp = g_Engine->Get<DrawCallService>()->GetGPUModelDataBuffer();
	auto l_MaterialGPUBufferComp = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();

	l_fmService->CommandListBegin(m_visualizationRenderPassComp, m_visualizationCommandListComp, 0);
	l_fmService->BindRenderPassComponent(m_visualizationRenderPassComp, m_visualizationCommandListComp);
	l_fmService->ClearRenderTargets(m_visualizationRenderPassComp, m_visualizationCommandListComp);

	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, m_SamplerComp, 4);

	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, input, 3);

	// for (uint32_t i = 0; i < l_drawCallCount; i++)
	// {
	// 	auto l_drawCallData = l_drawCallInfo[i];
	// 	auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::MainCamera);
	// 	if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
	// 	{
	// 		if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
	// 		{
	// 			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
	// 			{
	// 				l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.m_PerObjectConstantBufferIndex, 1);
	// 				l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.m_PerObjectConstantBufferIndex, 1);

	// 				l_fmService->DrawIndexedInstanced(m_visualizationRenderPassComp, l_drawCallData.mesh);
	// 			}
	// 		}
	// 	}
	// }

	// l_fmService->CommandListEnd(m_visualizationRenderPassComp, m_visualizationCommandListComp);

	return true;
}

bool VolumetricPass::ExecuteCommands(bool visualize)
{
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	froxelization();
	irraidanceInjection();
	rayMarching();

	if (visualize)
	{
		visualization(m_rayMarchingResult_A);
	}

	// Note: Command lists are prepared in the functions above but currently commented out
	// TODO: Implement proper command list preparation and execution
	// auto l_cmdList1 = froxelizationCommandList();
	// if (l_cmdList1) { l_graphicsService->Execute(l_cmdList1, GPUEngineType::Graphics); }
	l_hwService->WaitOnGPU(m_froxelizationRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	l_hwService->WaitOnGPU(m_froxelizationRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);

	// auto l_cmdList2 = irraidanceInjectionCommandList();
	// if (l_cmdList2) { l_graphicsService->Execute(l_cmdList2, GPUEngineType::Graphics); }
	l_hwService->WaitOnGPU(m_irraidanceInjectionRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);
	// if (l_cmdList2) { l_graphicsService->Execute(l_cmdList2, GPUEngineType::Compute); }
	l_hwService->WaitOnGPU(m_irraidanceInjectionRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Compute);

	// auto l_cmdList3 = rayMarchingCommandList();
	// if (l_cmdList3) { l_graphicsService->Execute(l_cmdList3, GPUEngineType::Graphics); }
	l_hwService->WaitOnGPU(m_rayMarchingRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);
	// if (l_cmdList3) { l_graphicsService->Execute(l_cmdList3, GPUEngineType::Compute); }
	l_hwService->WaitOnGPU(m_rayMarchingRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Compute);

	if (visualize)
	{
		// auto l_cmdList4 = visualizationCommandList();
		// if (l_cmdList4) { l_graphicsService->Execute(l_cmdList4, GPUEngineType::Graphics); }
		l_hwService->WaitOnGPU(m_visualizationRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	}

	return true;
}

#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../GPUBufferResourceService.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

#include "../../Services/TemplateAssetService.h"
#include "../../Component/GPUResourceCast.h"

#include "../../Engine.h"

using namespace Inno;

bool FrameManagementService::PrepareGlobalCommands()
{
	auto l_currentFrame = m_CurrentFrame;

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	Open(l_commandList, GPUEngineType::Graphics);

	auto l_gpuBufferService = g_Engine->Get<GPUBufferResourceService>();
	l_gpuBufferService->ForEach([&](GPUBufferComponent* i)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			return;
		if (i->m_MappedMemories.size() == 0)
			return;

		auto l_mappedMemory = i->m_MappedMemories[l_currentFrame];
		if (l_mappedMemory->m_NeedUploadToGPU)
		{
			TryToTransitState(i, l_commandList, Accessibility::ReadOnly, Accessibility::CopyDestination);
			l_gpuBufferService->UploadToGPU(l_commandList, i);
			TryToTransitState(i, l_commandList, Accessibility::CopyDestination, Accessibility::ReadOnly);
			l_mappedMemory->m_NeedUploadToGPU = false;
		}
	});

	l_gpuBufferService->UpdateRaytracingInstances();

	PrepareRayTracing(l_commandList);

	Close(l_commandList, GPUEngineType::Graphics);

	return true;
}

bool FrameManagementService::ExecuteGlobalCommands()
{
	auto l_currentFrame = m_CurrentFrame;

	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	m_HardwareService->Execute(l_commandList, GPUEngineType::Graphics);
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);

	return true;
}

bool FrameManagementService::PrepareSwapChainCommands()
{
	if (g_Engine->getInitConfig().isOffscreen)
		return true;

	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_currentFrame = m_CurrentFrame;
	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	auto l_swapChainRP = m_SwapChainRenderPassComp;

	CommandListBegin(l_swapChainRP, l_commandList, l_currentFrame);

	if (auto* l_outputTexture = l_userPipelineOutput->As<TextureComponent>())
		TryToTransitState(l_outputTexture, l_commandList, Accessibility::WriteOnly, Accessibility::ReadOnly);
	BindRenderPassComponent(l_swapChainRP, l_commandList);

	ClearRenderTargets(l_swapChainRP, l_commandList);

	BindGPUResource(l_swapChainRP, l_commandList, ShaderStage::Pixel, l_userPipelineOutput, 0);
	BindGPUResource(l_swapChainRP, l_commandList, ShaderStage::Pixel, m_SwapChainSamplerComp, 1);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Square);

	DrawIndexedInstanced(l_swapChainRP, l_commandList, l_mesh, 1);

	TryToTransitState(l_swapChainRP->m_OutputMergerTarget->m_ColorOutputs[0], l_commandList, Accessibility::WriteOnly, Accessibility::ReadOnly);

	CommandListEnd(l_swapChainRP, l_commandList);

	return true;
}

bool FrameManagementService::ExecuteSwapChainCommands()
{
	if (g_Engine->getInitConfig().isOffscreen)
		return true;

	auto l_userPipelineOutput = m_GetUserPipelineOutputFunc();
	if (!l_userPipelineOutput)
		return false;

	if (l_userPipelineOutput->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_currentFrame = m_CurrentFrame;
	auto l_commandList = m_GlobalGraphicsCommandLists[l_currentFrame];
	m_HardwareService->Execute(l_commandList, GPUEngineType::Graphics);

	return true;
}

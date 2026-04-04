#include "DX12FrameManagementService.h"
#include "../IGraphicsService.h"

using namespace Inno;

uint32_t DX12FrameManagementService::GetCurrentFrame()
{
	return m_Backend->GetCurrentFrame();
}

uint32_t DX12FrameManagementService::GetPreviousFrame()
{
	return m_Backend->GetPreviousFrame();
}

uint32_t DX12FrameManagementService::GetNextFrame()
{
	return m_Backend->GetNextFrame();
}

uint32_t DX12FrameManagementService::GetSwapChainImageCount()
{
	return m_Backend->GetSwapChainImageCount();
}

uint32_t DX12FrameManagementService::GetFrameCountSinceLaunch()
{
	return m_Backend->GetFrameCountSinceLaunch();
}

void DX12FrameManagementService::SetUploadHeapPreparationCallback(std::function<bool()>&& callback)
{
	m_Backend->SetUploadHeapPreparationCallback(std::move(callback));
}

void DX12FrameManagementService::SetCommandPreparationCallback(std::function<bool()>&& callback)
{
	m_Backend->SetCommandPreparationCallback(std::move(callback));
}

void DX12FrameManagementService::SetCommandExecutionCallback(std::function<bool()>&& callback)
{
	m_Backend->SetCommandExecutionCallback(std::move(callback));
}

RenderPassComponent* DX12FrameManagementService::GetSwapChainRenderPassComponent()
{
	return m_Backend->GetSwapChainRenderPassComponent();
}

bool DX12FrameManagementService::Resize()
{
	return m_Backend->Resize();
}

bool DX12FrameManagementService::Present()
{
	return m_Backend->Present();
}

bool DX12FrameManagementService::SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& func)
{
	return m_Backend->SetUserPipelineOutput(std::move(func));
}

GPUResourceComponent* DX12FrameManagementService::GetUserPipelineOutput()
{
	return m_Backend->GetUserPipelineOutput();
}

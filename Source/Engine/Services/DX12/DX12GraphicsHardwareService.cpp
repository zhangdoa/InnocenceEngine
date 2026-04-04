#include "DX12GraphicsHardwareService.h"
#include "../IGraphicsService.h"

using namespace Inno;

bool DX12GraphicsHardwareService::SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType)
{
	return m_Backend->SignalOnGPU(semaphore, queueType);
}

bool DX12GraphicsHardwareService::WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	return m_Backend->WaitOnGPU(semaphore, queueType, semaphoreType);
}

bool DX12GraphicsHardwareService::Execute(CommandListComponent* commandList, GPUEngineType queueType)
{
	return m_Backend->Execute(commandList, queueType);
}

uint64_t DX12GraphicsHardwareService::GetSemaphoreValue(GPUEngineType queueType)
{
	return m_Backend->GetSemaphoreValue(queueType);
}

bool DX12GraphicsHardwareService::WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType)
{
	return m_Backend->WaitOnCPU(semaphoreValue, queueType);
}

bool DX12GraphicsHardwareService::Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject)
{
	return m_Backend->Open(commandList, engineType, pipelineStateObject);
}

bool DX12GraphicsHardwareService::Close(CommandListComponent* commandList, GPUEngineType engineType)
{
	return m_Backend->Close(commandList, engineType);
}

bool DX12GraphicsHardwareService::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	return m_Backend->CommandListBegin(renderPass, commandList, frameIndex);
}

bool DX12GraphicsHardwareService::BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	return m_Backend->BindRenderPassComponent(renderPass, commandList);
}

bool DX12GraphicsHardwareService::ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index)
{
	return m_Backend->ClearRenderTargets(renderPass, commandList, index);
}

bool DX12GraphicsHardwareService::BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return m_Backend->BindGPUResource(renderPass, commandList, shaderStage, resource, resourceBindingLayoutDescIndex, startOffset, elementCount);
}

bool DX12GraphicsHardwareService::UnbindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return m_Backend->UnbindGPUResource(renderPass, commandList, shaderStage, resource, resourceBindingLayoutDescIndex, startOffset, elementCount);
}

bool DX12GraphicsHardwareService::TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	return m_Backend->TryToTransitState(texture, commandList, sourceAccessibility, targetAccessibility);
}

bool DX12GraphicsHardwareService::TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	return m_Backend->TryToTransitState(gpuBuffer, commandList, sourceAccessibility, targetAccessibility);
}

bool DX12GraphicsHardwareService::DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount)
{
	return m_Backend->DrawIndexedInstanced(renderPass, commandList, mesh, instanceCount);
}

bool DX12GraphicsHardwareService::DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount)
{
	return m_Backend->DrawInstanced(renderPass, commandList, instanceCount);
}

bool DX12GraphicsHardwareService::Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	return m_Backend->Dispatch(renderPass, commandList, threadGroupX, threadGroupY, threadGroupZ);
}

bool DX12GraphicsHardwareService::DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ)
{
	return m_Backend->DispatchRays(renderPass, commandList, dimensionX, dimensionY, dimensionZ);
}

bool DX12GraphicsHardwareService::ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand)
{
	return m_Backend->ExecuteIndirect(renderPass, commandList, indirectDrawCommand);
}

void DX12GraphicsHardwareService::PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants)
{
	m_Backend->PushRootConstants(renderPass, commandList, rootConstants);
}

bool DX12GraphicsHardwareService::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	return m_Backend->CommandListEnd(renderPass, commandList);
}

bool DX12GraphicsHardwareService::BeginCapture()
{
	return m_Backend->BeginCapture();
}

bool DX12GraphicsHardwareService::EndCapture()
{
	return m_Backend->EndCapture();
}

bool DX12GraphicsHardwareService::HasGPUError() const
{
	return m_Backend->HasGPUError();
}

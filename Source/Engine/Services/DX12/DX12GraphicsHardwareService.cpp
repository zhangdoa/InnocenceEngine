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

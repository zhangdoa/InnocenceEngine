#include "DX12GraphicsHardwareService.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

#include <Windows.h>

using namespace Inno;
using namespace DX12Helper;

bool DX12GraphicsHardwareService::WaitOnFenceWithDiagnostics(const char* fenceName, ID3D12Fence* fence, HANDLE fenceEvent, uint64_t semaphoreValue)
{
	if (fence->GetCompletedValue() >= semaphoreValue)
		return true;

	fence->SetEventOnCompletion(semaphoreValue, fenceEvent);
	DWORD l_waitResult = WaitForSingleObject(fenceEvent, 30000);
	if (l_waitResult == WAIT_OBJECT_0)
		return true;

	auto l_drr = m_DX12Context.m_device ? m_DX12Context.m_device->GetDeviceRemovedReason() : S_OK;
	if (l_waitResult == WAIT_TIMEOUT)
	{
		Log(Error, fenceName, " wait timeout (30s). Semaphore=", semaphoreValue,
			" Completed=", fence->GetCompletedValue(),
			" DeviceRemovedReason=", static_cast<int32_t>(l_drr));
	}
	else
	{
		Log(Error, fenceName, " wait failed. WaitResult=", static_cast<uint32_t>(l_waitResult),
			" LastError=", static_cast<uint32_t>(GetLastError()),
			" DeviceRemovedReason=", static_cast<int32_t>(l_drr));
	}

	if (l_drr != S_OK)
	{
		m_DX12Context.m_GPUErrorDetected.store(true);
		DumpDRED(m_DX12Context.m_device.Get());
	}
	return false;
}

ComPtr<ID3D12Device8> DX12GraphicsHardwareService::GetDevice()
{
	return m_DX12Context.m_device.Get();
}

ComPtr<ID3D12CommandAllocator> DX12GraphicsHardwareService::GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
{
	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	return m_DX12Context.GetGlobalCommandAllocator(commandListType, l_currentFrame);
}

ComPtr<ID3D12CommandQueue> DX12GraphicsHardwareService::GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return m_DX12Context.GetGlobalCommandQueue(commandListType);
}

DX12DescriptorHeapAccessor& DX12GraphicsHardwareService::GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility,
	Accessibility resourceAccessibility, TextureUsage textureUsage, bool isShaderVisible)
{
	return m_DX12Context.GetDescriptorHeapAccessor(type, bindingAccessibility, resourceAccessibility, textureUsage, isShaderVisible);
}

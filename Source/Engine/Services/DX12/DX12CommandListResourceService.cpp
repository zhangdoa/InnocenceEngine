#include "DX12CommandListResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

using namespace Inno;

bool DX12CommandListResourceService::Delete(CommandListComponent* ptr)
{
	auto l_dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(ptr->m_CommandList);
	if (l_dx12CommandList)
		l_dx12CommandList->Release();

	CommandListResourceService::Delete(ptr);
	return true;
}

bool DX12CommandListResourceService::InitializeImpl(CommandListComponent* commandList)
{
	if (!commandList)
	{
		Log(Error, "CommandList parameter is null");
		return false;
	}

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	ComPtr<ID3D12GraphicsCommandList7> l_commandList;
	HRESULT l_HResult;

	switch (commandList->m_Type)
	{
	case GPUEngineType::Graphics:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Compute:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Copy:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	default:
		Log(Error, commandList->m_InstanceName, " Unknown GPU engine type for command list creation");
		return false;
	}

	if (FAILED(l_HResult))
	{
		Log(Error, commandList->m_InstanceName, " Failed to create DX12 command list");
		return false;
	}

	l_HResult = l_commandList->Close();
	if (FAILED(l_HResult))
	{
		Log(Error, commandList->m_InstanceName, " Failed to close command list after creation");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(commandList, l_commandList, "CommandList");
#endif

	commandList->m_CommandList = reinterpret_cast<uint64_t>(l_commandList.Detach());
	commandList->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, commandList->m_InstanceName, " Command list created successfully");
	return true;
}

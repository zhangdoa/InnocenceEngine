#include "DX12TextureResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "../FrameManagementService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12TextureResourceService::Delete(TextureComponent* texture)
{
	auto componentUUID = reinterpret_cast<uint64_t>(texture);

	auto uploadIt = m_TextureBuffers_Upload.find(componentUUID);
	if (uploadIt != m_TextureBuffers_Upload.end()) {
		if (uploadIt->second) uploadIt->second.Reset();
		m_TextureBuffers_Upload.erase(uploadIt);
	}

	auto defaultIt = m_TextureBuffers_Default.find(componentUUID);
	if (defaultIt != m_TextureBuffers_Default.end()) {
		for (auto& buf : defaultIt->second) buf.Reset();
		m_TextureBuffers_Default.erase(defaultIt);
	}

	texture->m_GPUResources.clear();
	texture->m_ReadHandles.clear();
	texture->m_WriteHandles.clear();

	TextureResourceService::Delete(texture);
	return true;
}

bool DX12TextureResourceService::Clear(CommandListComponent* commandList, TextureComponent* texture)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	uint32_t frameIndex = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));
	if (!resource)
	{
		Log(Error, "Cannot find texture resource for clear operation");
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	uint32_t handleIndex = texture->GetHandleIndex(frameIndex, 0);
	if (texture->m_TextureDesc.PixelDataType < TexturePixelDataType::Float16)
	{
		l_commandList->ClearUnorderedAccessViewUint(
			D3D12_GPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_GPUHandle },
			D3D12_CPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_CPUHandle },
			resource,
			(UINT*)&texture->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}
	else
	{
		l_commandList->ClearUnorderedAccessViewFloat(
			D3D12_GPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_GPUHandle },
			D3D12_CPU_DESCRIPTOR_HANDLE{ texture->m_WriteHandles[handleIndex].m_CPUHandle },
			resource,
			&texture->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}

	return true;
}

bool DX12TextureResourceService::Copy(CommandListComponent* commandList, TextureComponent* sourceTexture, TextureComponent* destinationTexture)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	uint32_t frameIndex = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	auto* srcResource = static_cast<ID3D12Resource*>(sourceTexture->GetGPUResource(frameIndex));
	auto* destResource = static_cast<ID3D12Resource*>(destinationTexture->GetGPUResource(frameIndex));

	if (!srcResource || !destResource)
	{
		Log(Error, "Cannot find texture resources for copy operation");
		return false;
	}

	l_commandList->CopyResource(destResource, srcResource);

	return true;
}

bool DX12TextureResourceService::CreateSRV(TextureComponent* texture, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	auto l_desc = GetSRVDesc(texture->m_TextureDesc, l_textureDesc, mipSlice);
	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadOnly, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage);

	uint32_t frameCount = texture->m_GPUResources.size();
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		auto* resource = static_cast<ID3D12Resource*>(texture->m_GPUResources[frame]);
		if (!resource)
		{
			Log(Error, texture->m_InstanceName, " No GPU resource found for frame ", frame);
			return false;
		}

		uint32_t handleIndex = texture->GetHandleIndex(frame, mipSlice);
		texture->m_ReadHandles[handleIndex] = l_descHeapAccessor.GetNewHandle();
		m_ctx->m_device->CreateShaderResourceView(resource,
			&l_desc,
			D3D12_CPU_DESCRIPTOR_HANDLE{ texture->m_ReadHandles[handleIndex].m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", texture->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12TextureResourceService::CreateUAV(TextureComponent* texture, uint32_t mipSlice)
{
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	auto l_desc = GetUAVDesc(texture->m_TextureDesc, l_textureDesc, mipSlice);

	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadWrite, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage);
	auto& l_descHeapAccessor_ShaderNonVisible = m_ctx->GetDescriptorHeapAccessor(texture->m_GPUResourceType, Accessibility::ReadWrite, texture->m_GPUAccessibility, texture->m_TextureDesc.Usage, false);

	uint32_t frameCount = texture->m_GPUResources.size();
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		auto* resource = static_cast<ID3D12Resource*>(texture->m_GPUResources[frame]);
		if (!resource)
		{
			Log(Error, texture->m_InstanceName, " No GPU resource found for frame ", frame);
			return false;
		}

		uint32_t handleIndex = texture->GetHandleIndex(frame, mipSlice);

		auto l_descHandle = l_descHeapAccessor.GetNewHandle();
		auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

		texture->m_WriteHandles[handleIndex].m_CPUHandle = l_descHandle_ShaderNonVisible.m_CPUHandle;
		texture->m_WriteHandles[handleIndex].m_GPUHandle = l_descHandle.m_GPUHandle;
		texture->m_WriteHandles[handleIndex].m_Index = l_descHandle.m_Index;

		m_ctx->m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_ctx->m_device->CreateUnorderedAccessView(resource, 0,
			&l_desc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", handleIndex, " for ", texture->m_InstanceName, " has been created.");
	}

	return true;
}

std::optional<uint32_t> DX12TextureResourceService::GetIndex(TextureComponent* texture, Accessibility bindingAccessibility)
{
    if (!texture)
        return std::nullopt;

    if (texture->m_ObjectStatus != ObjectStatus::Activated)
        return std::nullopt;

    auto l_handleIndex = texture->m_TextureDesc.IsMultiBuffer ? g_Engine->Get<FrameManagementService>()->GetCurrentFrame() : 0;

    if (bindingAccessibility == Accessibility::ReadOnly)
    {
        if (l_handleIndex < texture->m_ReadHandles.size())
            return texture->m_ReadHandles[l_handleIndex].m_Index;
    }
    else if (bindingAccessibility.CanWrite())
    {
        if (l_handleIndex < texture->m_WriteHandles.size())
            return texture->m_WriteHandles[l_handleIndex].m_Index;
    }

    return std::nullopt;
}

#include "DX12TextureResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "DX12Helper_Pipeline.h"
#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/MathHelper.h"
#include "../../Engine.h"

#ifdef max
#undef max
#endif

using namespace Inno;
using namespace DX12Helper;

// ---------------------------------------------------------------------------
// Delete
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// InitializeImpl
// ---------------------------------------------------------------------------

bool DX12TextureResourceService::InitializeImpl(TextureComponent* texture, void* textureData)
{
	texture->m_GPUResourceType = GPUResourceType::Image;
	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	texture->m_WriteState = GetTextureWriteState(texture->m_TextureDesc);
	texture->m_ReadState = GetTextureReadState(texture->m_TextureDesc);

	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
	auto stateFrameCount = texture->m_TextureDesc.IsMultiBuffer ? l_swapChainImageCount : 1;
	auto l_initialState = static_cast<D3D12_RESOURCE_STATES>(texture->m_TextureDesc.Usage == TextureUsage::Sample ? texture->m_ReadState : texture->m_WriteState);
	
	if (texture->m_TextureDesc.UseSharedHandle)
	{
		l_initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	texture->m_CurrentState.resize(stateFrameCount, l_initialState);

	D3D12_CLEAR_VALUE l_clearValue = {};
	bool useClearValue = false;
	if (texture->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		useClearValue = true;
	}
	else if (texture->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		useClearValue = true;
	}
	else if (texture->m_TextureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_clearValue.Format = l_textureDesc.Format;
		l_clearValue.Color[0] = texture->m_TextureDesc.ClearColor[0];
		l_clearValue.Color[1] = texture->m_TextureDesc.ClearColor[1];
		l_clearValue.Color[2] = texture->m_TextureDesc.ClearColor[2];
		l_clearValue.Color[3] = texture->m_TextureDesc.ClearColor[3];
		useClearValue = true;
	}

	uint32_t frameCount = texture->m_TextureDesc.IsMultiBuffer ? l_swapChainImageCount : 1;
	texture->m_GPUResources.resize(frameCount);

	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		ComPtr<ID3D12Resource> defaultHeapBuffer;
		if (useClearValue)
			defaultHeapBuffer = m_ctx->CreateDefaultHeapBuffer(&l_textureDesc, l_initialState, &l_clearValue, texture->m_TextureDesc.UseSharedHandle);
		else
			defaultHeapBuffer = m_ctx->CreateDefaultHeapBuffer(&l_textureDesc, l_initialState, nullptr, texture->m_TextureDesc.UseSharedHandle);

		if (!defaultHeapBuffer)
		{
			Log(Error, texture->m_InstanceName, " Failed to create default heap buffer for frame ", frame);
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(texture, defaultHeapBuffer, ("DefaultHeap_Texture_Frame" + std::to_string(frame)).c_str());
#endif

		texture->m_GPUResources[frame] = defaultHeapBuffer.Get();
		m_TextureBuffers_Default[reinterpret_cast<uint64_t>(texture)].push_back(std::move(defaultHeapBuffer));
	}

	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();

	// Phase 1: Upload texture data with direct command list
	if (textureData)
	{
		ComPtr<ID3D12CommandAllocator> l_uploadAllocator;
		if (FAILED(m_ctx->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&l_uploadAllocator))))
		{
			Log(Error, texture->m_InstanceName, " Failed to create temporary command allocator for texture upload!");
			return false;
		}
		CommandListComponent l_uploadCommandList = {};
		l_uploadCommandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12UploadCommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, l_uploadAllocator, L"TextureUploadCommandList");
		l_uploadCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12UploadCommandList.Get());

		auto* defaultHeapBuffer_Frame0 = static_cast<ID3D12Resource*>(texture->m_GPUResources[0]);
		uint32_t l_subresourcesCount = texture->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(defaultHeapBuffer_Frame0, 0, l_subresourcesCount);

		auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
		auto l_uploadHeapBuffer = m_ctx->CreateUploadHeapBuffer(&l_resourceDesc);
		if (!l_uploadHeapBuffer)
		{
			Log(Error, texture->m_InstanceName, " Failed to create upload heap buffer for frame 0");
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(texture, l_uploadHeapBuffer, "UploadHeap_Texture");
#endif

		m_TextureBuffers_Upload[reinterpret_cast<uint64_t>(texture)] = l_uploadHeapBuffer;

		D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
		if (texture->m_TextureDesc.PixelDataType == TexturePixelDataType::Compressed)
		{
			l_textureSubResourceData.RowPitch = GetBCRowPitch(texture->m_TextureDesc);
			uint32_t blocksPerCol = (texture->m_TextureDesc.Height + 3) / 4;
			l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * blocksPerCol;
		}
		else
		{
			l_textureSubResourceData.RowPitch = texture->m_TextureDesc.Width * GetTexturePixelDataSize(texture->m_TextureDesc);
			l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * texture->m_TextureDesc.Height;
		}
		l_textureSubResourceData.pData = (unsigned char*)textureData;

		for (auto gpuResource : texture->m_GPUResources)
		{
			auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(gpuResource);
			l_dx12UploadCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer, l_initialState, D3D12_RESOURCE_STATE_COPY_DEST));

			UpdateSubresources(l_dx12UploadCommandList.Get(), l_defaultHeapBuffer, l_uploadHeapBuffer.Get(), 0, 0, l_subresourcesCount, &l_textureSubResourceData);

			auto l_nextState = texture->m_TextureDesc.MipLevels > 1 ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : l_initialState;
			l_dx12UploadCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer, D3D12_RESOURCE_STATE_COPY_DEST, l_nextState));
		}

		// Execute and wait for upload phase
		l_fmService->Close(&l_uploadCommandList, GPUEngineType::Graphics);
		l_hwService->Execute(&l_uploadCommandList, GPUEngineType::Graphics);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
		auto l_uploadSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_uploadSemaphoreValue, GPUEngineType::Graphics);
	}

	// Create descriptor handles
	uint32_t mipLevels = l_textureDesc.MipLevels;
	texture->m_ReadHandles.resize(frameCount * mipLevels);
	texture->m_WriteHandles.resize(frameCount * mipLevels);
	for (uint32_t frame = 0; frame < frameCount; frame++)
	{
		for (uint32_t mip = 0; mip < mipLevels; mip++)
		{
			uint32_t handleIndex = texture->GetHandleIndex(frame, mip);
			if (!CreateSRV(texture, mip))
			{
				Log(Error, texture->m_InstanceName, " Failed to create SRV for frame ", frame, " mip ", mip);
				return false;
			}
		}
	}

	if (texture->m_TextureDesc.Usage != TextureUsage::DepthAttachment
		&& texture->m_TextureDesc.Usage != TextureUsage::DepthStencilAttachment
		&& !texture->m_TextureDesc.IsSRGB
		&& texture->m_TextureDesc.PixelDataType != TexturePixelDataType::Compressed)
	{
		for (uint32_t frame = 0; frame < frameCount; frame++)
		{
			for (uint32_t mip = 0; mip < mipLevels; mip++)
			{
				if (!CreateUAV(texture, mip))
				{
					Log(Error, texture->m_InstanceName, " Failed to create UAV for frame ", frame, " mip ", mip);
					return false;
				}
			}
		}
	}

	// Phase 2: Generate mipmaps with compute command list (if needed)
	if (texture->m_TextureDesc.MipLevels > 1 && textureData)
	{
		ComPtr<ID3D12CommandAllocator> l_mipmapAllocator;
		if (FAILED(m_ctx->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&l_mipmapAllocator))))
		{
			Log(Error, texture->m_InstanceName, " Failed to create temporary command allocator for mipmap generation!");
			return false;
		}
		CommandListComponent l_mipmapCommandList = {};
		l_mipmapCommandList.m_Type = GPUEngineType::Compute;
		auto l_dx12MipmapCommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, l_mipmapAllocator, L"TextureMipmapCommandList");
		l_mipmapCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12MipmapCommandList.Get());

		GenerateMipmap(texture, &l_mipmapCommandList);

		l_dx12MipmapCommandList->Close();
		l_hwService->Execute(&l_mipmapCommandList, GPUEngineType::Compute);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Compute);
		auto l_computeSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);

		// Phase 3: Transition texture back to initial state for rendering passes
		ComPtr<ID3D12CommandAllocator> l_transitionAllocator;
		if (FAILED(m_ctx->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&l_transitionAllocator))))
		{
			Log(Error, texture->m_InstanceName, " Failed to create temporary command allocator for texture state transition!");
			return false;
		}
		CommandListComponent l_transitionCommandList = {};
		l_transitionCommandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12TransitionCommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, l_transitionAllocator, L"TextureTransitionCommandList");
		l_transitionCommandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12TransitionCommandList.Get());

		for (auto gpuResource : texture->m_GPUResources)
		{
			auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(gpuResource);
			l_dx12TransitionCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				l_defaultHeapBuffer,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				l_initialState));
		}

		// Execute and wait for transition
		l_fmService->Close(&l_transitionCommandList, GPUEngineType::Graphics);
		l_hwService->Execute(&l_transitionCommandList, GPUEngineType::Graphics);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
		auto l_transitionSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_transitionSemaphoreValue, GPUEngineType::Graphics);
	}

	texture->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, texture->m_InstanceName, " is initialized.");

	return true;
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

bool DX12TextureResourceService::Clear(CommandListComponent* commandList, TextureComponent* texture)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
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

// ---------------------------------------------------------------------------
// Copy
// ---------------------------------------------------------------------------

bool DX12TextureResourceService::Copy(CommandListComponent* commandList, TextureComponent* sourceTexture, TextureComponent* destinationTexture)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
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

// ---------------------------------------------------------------------------
// GenerateMipmap
// ---------------------------------------------------------------------------

bool DX12TextureResourceService::GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList)
{
    if (!commandList)
    {
        Log(Error, "GenerateMipmap requires a valid command list for proper synchronization");
        return false;
    }

    if(texture->m_TextureDesc.IsSRGB)
    {
        Log(Warning, "SRGB mipmap generation not currently supported, skipping texture");
        return true;
    }

    struct DWParam
    {
        DWParam(FLOAT f) : Float(f) {}
        DWParam(UINT u) : Uint(u) {}

        void operator=(FLOAT f) { Float = f; }
        void operator=(UINT u) { Uint = u; }

        union
        {
            FLOAT Float;
            UINT Uint;
        };
    };

    if (texture->m_TextureDesc.MipLevels == 1)
    {
        Log(Warning, texture->m_InstanceName, " Attempt to generate mipmaps for texture without mipmaps requirement.");
        return false;
    }

    bool isStaticTexture = (texture->m_TextureDesc.Usage == TextureUsage::Sample);
    bool isRenderTarget = (texture->m_TextureDesc.Usage == TextureUsage::ColorAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::DepthAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::ComputeOnly);

    size_t startIndex = 0;
    size_t endIndex = 1;

    auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

    if (isStaticTexture && texture->m_TextureDesc.IsMultiBuffer)
    {
        endIndex = texture->m_ReadHandles.size();
    }
    else if (isRenderTarget && texture->m_TextureDesc.IsMultiBuffer)
    {
        startIndex = l_currentFrame;
        endIndex = startIndex + 1;
    }
    else
    {
        startIndex = 0;
        endIndex = 1;
    }

    auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
    if (!l_DX12CommandList)
    {
        Log(Error, texture->m_InstanceName, " Invalid command list");
        return false;
    }

    if (texture->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
    {
        l_DX12CommandList->SetComputeRootSignature(m_3DMipmapRootSignature);
        l_DX12CommandList->SetPipelineState(m_3DMipmapPSO);
    }
    else
    {
        l_DX12CommandList->SetComputeRootSignature(m_2DMipmapRootSignature);
        l_DX12CommandList->SetPipelineState(m_2DMipmapPSO);
    }

    ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get() };
    l_DX12CommandList->SetDescriptorHeaps(1, l_heaps);

    uint32_t l_mipLevels = texture->m_TextureDesc.MipLevels;

    for (size_t deviceMemoryIndex = startIndex; deviceMemoryIndex < endIndex; deviceMemoryIndex++)
    {
        auto l_defaultHeapBuffer = reinterpret_cast<ID3D12Resource*>(texture->m_GPUResources[deviceMemoryIndex]);
        if (!l_defaultHeapBuffer)
        {
            Log(Error, texture->m_InstanceName, " Invalid device memory at index ", deviceMemoryIndex);
            return false;
        }

        for (uint32_t mipLevel = 0; mipLevel < l_mipLevels - 1; mipLevel++)
        {
            uint32_t dstWidth = std::max(texture->m_TextureDesc.Width >> (mipLevel + 1), 1u);
            uint32_t dstHeight = std::max(texture->m_TextureDesc.Height >> (mipLevel + 1), 1u);
            uint32_t dstDepth = 1;

            l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
            l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

            if (texture->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
            {
                dstDepth = std::max(texture->m_TextureDesc.DepthOrArraySize >> (mipLevel + 1), 1u);
                l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
            }

            auto l_srcHandleIndex = texture->GetHandleIndex(deviceMemoryIndex, mipLevel);
            auto l_srcUAV = D3D12_GPU_DESCRIPTOR_HANDLE { texture->m_WriteHandles[l_srcHandleIndex].m_GPUHandle };

            auto l_dstHandleIndex = texture->GetHandleIndex(deviceMemoryIndex, mipLevel + 1);
            auto l_dstUAV = D3D12_GPU_DESCRIPTOR_HANDLE { texture->m_WriteHandles[l_dstHandleIndex].m_GPUHandle };

            l_DX12CommandList->SetComputeRootDescriptorTable(1, l_srcUAV);
            l_DX12CommandList->SetComputeRootDescriptorTable(2, l_dstUAV);

            uint32_t dispatchX = std::max(dstWidth / 8, 1u);
            uint32_t dispatchY = std::max(dstHeight / 8, 1u);
            uint32_t dispatchZ = std::max(dstDepth / 8, 1u);

            l_DX12CommandList->Dispatch(dispatchX, dispatchY, dispatchZ);

            auto l_uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_defaultHeapBuffer);
            l_DX12CommandList->ResourceBarrier(1, &l_uavBarrier);
        }
    }

    auto memoryCount = endIndex - startIndex;
    Log(Verbose, texture->m_InstanceName, " Successfully recorded mipmap generation commands for ", l_mipLevels, " mip levels for ", memoryCount, " device memory/memories");
    return true;
}

// ---------------------------------------------------------------------------
// SRV / UAV creation
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// GetIndex
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// ReadTextureBackToCPU
// ---------------------------------------------------------------------------

std::vector<Vec4> DX12TextureResourceService::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
    auto textureDesc = TextureComp->m_TextureDesc;
    auto l_frameIndex = textureDesc.IsMultiBuffer ? g_Engine->Get<FrameManagementService>()->GetCurrentFrame() : 0;

    if (l_frameIndex >= TextureComp->m_GPUResources.size())
    {
        Log(Error, TextureComp, " frame index ", l_frameIndex, " out of bounds (size: ", TextureComp->m_GPUResources.size(), ")");
        return {};
    }

    auto l_defaultHeapBuffer = static_cast<ID3D12Resource*>(TextureComp->m_GPUResources[l_frameIndex]);
    if (!l_defaultHeapBuffer)
    {
        Log(Error, TextureComp, " has null GPU resource at frame index ", l_frameIndex);
        return {};
    }

    auto l_srcDesc = l_defaultHeapBuffer->GetDesc();

    uint32_t l_subresourceCount = textureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : textureDesc.DepthOrArraySize;
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints(l_subresourceCount);
    m_ctx->m_device->GetCopyableFootprints(&l_srcDesc, 0, l_subresourceCount, 0, l_footprints.data(), NULL, NULL, NULL);

    UINT64 l_bufferSize = 0;
    for (uint32_t i = 0; i < l_subresourceCount; ++i)
        l_bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;

    auto l_readBackHeapBuffer = m_ctx->CreateReadBackHeapBuffer(l_bufferSize);
    if (!l_readBackHeapBuffer)
    {
        Log(Warning, TextureComp, " failed to create readback heap buffer");
        return {};
    }

    DXGI_FORMAT l_format = DX12Helper::GetTextureFormat(textureDesc);

    {
        auto l_beforeState = static_cast<D3D12_RESOURCE_STATES>(TextureComp->GetCurrentState(l_frameIndex));
        ComPtr<ID3D12CommandAllocator> l_tempAllocator;
        auto l_allocResult = m_ctx->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&l_tempAllocator));
        if (FAILED(l_allocResult))
        {
            Log(Error, TextureComp, " failed to create temporary command allocator for readback");
            return {};
        }
        auto l_dx12CommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, l_tempAllocator, L"ReadTextureBackToCPU_Transition");
        l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer, l_beforeState, D3D12_RESOURCE_STATE_COPY_SOURCE));

        for (uint32_t i = 0; i < l_subresourceCount; ++i)
        {
            D3D12_TEXTURE_COPY_LOCATION l_srcLocation = {};
            l_srcLocation.pResource = l_defaultHeapBuffer;
            l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            l_srcLocation.SubresourceIndex = i;

            D3D12_TEXTURE_COPY_LOCATION l_destLocation = {};
            l_destLocation.pResource = l_readBackHeapBuffer.Get();
            l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            l_destLocation.PlacedFootprint = l_footprints[i];
            l_destLocation.PlacedFootprint.Footprint.Format = l_format;

            l_dx12CommandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);
        }

        l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, l_beforeState));
        l_dx12CommandList->Close();

        auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
        auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();

        CommandListComponent l_commandListComp = {};
        l_commandListComp.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());
        l_hwService->Execute(&l_commandListComp, GPUEngineType::Graphics);
        l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
        l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
    }

    size_t l_pixelCount = 0;
    switch (textureDesc.Sampler)
    {
    case TextureSampler::Sampler1D:
        l_pixelCount = textureDesc.Width;
        break;
    case TextureSampler::Sampler2D:
        l_pixelCount = textureDesc.Width * textureDesc.Height;
        break;
    case TextureSampler::Sampler3D:
        l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
        break;
    case TextureSampler::Sampler1DArray:
        l_pixelCount = textureDesc.Width * textureDesc.DepthOrArraySize;
        break;
    case TextureSampler::Sampler2DArray:
        l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
        break;
    case TextureSampler::SamplerCubemap:
        l_pixelCount = textureDesc.Width * textureDesc.Height * 6;
        break;
    default:
        break;
    }

    if (l_pixelCount == 0)
    {
        Log(Error, TextureComp, " unsupported sampler type for readback: ", (uint32_t)textureDesc.Sampler);
        return {};
    }

    uint32_t l_pixelDataSize = DX12Helper::GetTexturePixelDataSize(textureDesc);
    std::vector<unsigned char> l_rawResult(l_bufferSize);

    CD3DX12_RANGE l_readRange(0, l_rawResult.size());
    void* l_pData = nullptr;
    auto l_hResult = l_readBackHeapBuffer->Map(0, &l_readRange, &l_pData);
    if (FAILED(l_hResult))
    {
        Log(Error, TextureComp, " failed to map readback heap buffer HRESULT=", l_hResult,
            " bufferSize=", l_rawResult.size(), " readRange=", l_readRange.End);
        auto l_drr = m_ctx->m_device->GetDeviceRemovedReason();
        Log(Error, "DeviceRemovedReason=", l_drr);
        return {};
    }
    std::memcpy(l_rawResult.data(), l_pData, l_rawResult.size());
    l_readBackHeapBuffer->Unmap(0, nullptr);
    std::vector<Vec4> l_result(l_pixelCount);
    size_t l_subresourceOffset = 0;
    for (uint32_t sub = 0; sub < l_subresourceCount; ++sub)
    {
        uint32_t l_rowPitch  = l_footprints[sub].Footprint.RowPitch;
        uint32_t l_subWidth  = l_footprints[sub].Footprint.Width;
        uint32_t l_subHeight = l_footprints[sub].Footprint.Height;
        uint32_t l_subDepth  = l_footprints[sub].Footprint.Depth;

        for (uint32_t z = 0; z < l_subDepth; ++z)
        {
            for (uint32_t row = 0; row < l_subHeight; ++row)
            {
                const unsigned char* l_srcRow = l_rawResult.data() + l_subresourceOffset + (z * l_subHeight + row) * l_rowPitch;
                for (uint32_t col = 0; col < l_subWidth; ++col)
                {
                    const unsigned char* l_PixelData = l_srcRow + col * l_pixelDataSize;

                    size_t l_dstIndex = 0;
                    switch (textureDesc.Sampler)
                    {
                    case TextureSampler::Sampler1D:
                        l_dstIndex = col;
                        break;
                    case TextureSampler::Sampler2D:
                        l_dstIndex = row * textureDesc.Width + col;
                        break;
                    case TextureSampler::Sampler3D:
                        l_dstIndex = (z * textureDesc.Height + row) * textureDesc.Width + col;
                        break;
                    case TextureSampler::Sampler1DArray:
                        l_dstIndex = sub * textureDesc.Width + col;
                        break;
                    case TextureSampler::Sampler2DArray:
                        l_dstIndex = (sub * textureDesc.Height + row) * textureDesc.Width + col;
                        break;
                    case TextureSampler::SamplerCubemap:
                        l_dstIndex = (sub * textureDesc.Height + row) * textureDesc.Width + col;
                        break;
                    default:
                        break;
                    }

                    if (textureDesc.PixelDataType == TexturePixelDataType::Float32)
                    {
                        float r, g, b, a;
                        memcpy(&r, l_PixelData + 0,  4);
                        memcpy(&g, l_PixelData + 4,  4);
                        memcpy(&b, l_PixelData + 8,  4);
                        memcpy(&a, l_PixelData + 12, 4);
                        l_result[l_dstIndex] = Vec4(r, g, b, a);
                    }
                    else if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
                    {
                        uint32_t channels = l_pixelDataSize / 2;
                        float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                        for (uint32_t ch = 0; ch < channels && ch < 4; ++ch)
                        {
                            uint16_t h;
                            memcpy(&h, l_PixelData + ch * 2, 2);
                            values[ch] = Math::float16ToFloat32(h);
                        }
                        l_result[l_dstIndex] = Vec4(values[0], values[1], values[2], values[3]);
                    }
                    else
                    {
                        l_result[l_dstIndex] = Vec4(l_PixelData[0] / 255.0f,
                                                    l_PixelData[1] / 255.0f,
                                                    l_PixelData[2] / 255.0f,
                                                    l_PixelData[3] / 255.0f);
                    }
                }
            }
        }
        l_subresourceOffset += l_rowPitch * l_subHeight * l_subDepth;
    }

    return l_result;
}

// ---------------------------------------------------------------------------
// CreateMipmapGenerator / ReleaseMipmapGenerator
// ---------------------------------------------------------------------------

bool DX12TextureResourceService::CreateMipmapGenerator()
{
	{
		CD3DX12_DESCRIPTOR_RANGE uavRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		uavRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);  // Source UAV
		uavRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);  // Destination UAV
		rootParameters[0].InitAsConstants(3, 0);
		rootParameters[1].InitAsDescriptorTable(1, &uavRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &uavRanges[1]);

		ID3DBlob* signature;
		ID3DBlob* error;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_ctx->m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_3DMipmapRootSignature));

		ShaderFilePath l_3DPath = "mipmapGenerator3D.comp";

		D3D12_COMPUTE_PIPELINE_STATE_DESC l_3DPSODesc = {};
		l_3DPSODesc.pRootSignature = m_3DMipmapRootSignature;

#ifdef USE_DXIL
		std::vector<uint8_t> l_3DmipmapComputeShader;
		LoadShaderFile(l_3DmipmapComputeShader, l_3DPath);
		l_3DPSODesc.CS = { l_3DmipmapComputeShader.data(), l_3DmipmapComputeShader.size() };
#else
		ID3DBlob* l_3DmipmapComputeShader;
		LoadShaderFile(&l_3DmipmapComputeShader, ShaderStage::Compute, l_3DPath);
		l_3DPSODesc.CS = { reinterpret_cast<UINT8*>(l_3DmipmapComputeShader->GetBufferPointer()), l_3DmipmapComputeShader->GetBufferSize() };
#endif
		m_ctx->m_device->CreateComputePipelineState(&l_3DPSODesc, IID_PPV_ARGS(&m_3DMipmapPSO));

		Log(Success, "Mipmap generator for 3D texture has been created.");
	}
	{
		CD3DX12_DESCRIPTOR_RANGE uavRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		uavRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);  // Source UAV
		uavRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);  // Destination UAV
		rootParameters[0].InitAsConstants(2, 0);
		rootParameters[1].InitAsDescriptorTable(1, &uavRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &uavRanges[1]);

		ID3DBlob* signature;
		ID3DBlob* error;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_ctx->m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_2DMipmapRootSignature));

		ShaderFilePath l_2DPath = "mipmapGenerator2D.comp";
		D3D12_COMPUTE_PIPELINE_STATE_DESC l_2DPSODesc = {};
		l_2DPSODesc.pRootSignature = m_2DMipmapRootSignature;

#ifdef USE_DXIL
		std::vector<uint8_t> l_2DmipmapComputeShader;
		LoadShaderFile(l_2DmipmapComputeShader, l_2DPath);
		l_2DPSODesc.CS = { l_2DmipmapComputeShader.data(), l_2DmipmapComputeShader.size() };
#else
		ID3DBlob* l_2DmipmapComputeShader;
		LoadShaderFile(&l_2DmipmapComputeShader, ShaderStage::Compute, l_2DPath);
		l_2DPSODesc.CS = { reinterpret_cast<UINT8*>(l_2DmipmapComputeShader->GetBufferPointer()), l_2DmipmapComputeShader->GetBufferSize() };
#endif
		m_ctx->m_device->CreateComputePipelineState(&l_2DPSODesc, IID_PPV_ARGS(&m_2DMipmapPSO));

		Log(Success, "Mipmap generator for 2D texture has been created.");
	}

	return true;
}

bool DX12TextureResourceService::ReleaseMipmapGenerator()
{
	m_3DMipmapPSO->Release();
	m_2DMipmapPSO->Release();
	m_3DMipmapRootSignature->Release();
	m_2DMipmapRootSignature->Release();

	return true;
}

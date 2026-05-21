#include "DX12TextureResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

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

		l_fmService->Close(&l_uploadCommandList, GPUEngineType::Graphics);
		l_hwService->Execute(&l_uploadCommandList, GPUEngineType::Graphics);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
		auto l_uploadSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_uploadSemaphoreValue, GPUEngineType::Graphics);
	}

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

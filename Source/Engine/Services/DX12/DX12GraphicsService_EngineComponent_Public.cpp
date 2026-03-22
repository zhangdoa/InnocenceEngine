#include "DX12GraphicsService.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

#ifdef max
#undef max
#endif

#include "../../Engine.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"

using namespace Inno;
using namespace DX12Helper;

std::optional<uint32_t> DX12GraphicsService::GetIndex(TextureComponent* texture, Accessibility bindingAccessibility)
{
    if (!texture)
        return std::nullopt;
    
    if (texture->m_ObjectStatus != ObjectStatus::Activated)
        return std::nullopt;

    // Use proper handle index based on IsMultiBuffer flag
    auto l_handleIndex = texture->m_TextureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;
    
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

Vec4 DX12GraphicsService::ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y)
{
    return Vec4();
}

std::vector<Vec4> DX12GraphicsService::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
    auto textureDesc = TextureComp->m_TextureDesc;
    auto l_frameIndex = textureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;

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
    GetDevice()->GetCopyableFootprints(&l_srcDesc, 0, l_subresourceCount, 0, l_footprints.data(), NULL, NULL, NULL);

    UINT64 l_bufferSize = 0;
    for (uint32_t i = 0; i < l_subresourceCount; ++i)
        l_bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;

    auto l_readBackHeapBuffer = CreateReadBackHeapBuffer(l_bufferSize);
    if (!l_readBackHeapBuffer)
    {
        Log(Error, TextureComp, " failed to create readback heap buffer");
        return {};
    }

    DXGI_FORMAT l_format = DX12Helper::GetTextureFormat(textureDesc);

    {
        auto l_beforeState = DX12Helper::GetTextureWriteState(textureDesc);
        // Use a dedicated allocator so this temporary CL does not share the global
        // per-frame allocator, which has already been used by PrepareGlobalCommands.
        // Sharing would leave the allocator in a state that makes the next frame's
        // Open() (Reset) fail silently, causing EXECUTECOMMANDLISTS_FAILEDCOMMANDLIST.
        ComPtr<ID3D12CommandAllocator> l_tempAllocator;
        auto l_allocResult = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&l_tempAllocator));
        if (FAILED(l_allocResult))
        {
            Log(Error, TextureComp, " failed to create temporary command allocator for readback");
            return {};
        }
        auto l_dx12CommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, l_tempAllocator, L"ReadTextureBackToCPU_Transition");
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

        CommandListComponent l_commandListComp = {};
        l_commandListComp.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());
        Execute(&l_commandListComp, GPUEngineType::Graphics);
        SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
        WaitOnCPU(GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
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
        Log(Error, TextureComp, " failed to map readback heap buffer");
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

bool DX12GraphicsService::GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList)
{
    if (!commandList)
    {
        Log(Error, "GenerateMipmap requires a valid command list for proper synchronization");
        return false;
    }
    
    // Skip SRGB textures for now due to complexity - focus on core functionality
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


    // Determine if this is a static texture (Sample) or render target (attachment or compute usage)
    bool isStaticTexture = (texture->m_TextureDesc.Usage == TextureUsage::Sample);
    bool isRenderTarget = (texture->m_TextureDesc.Usage == TextureUsage::ColorAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::DepthAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment ||
        texture->m_TextureDesc.Usage == TextureUsage::ComputeOnly);

    // For static textures: generate mipmaps for all device memories
    // For render targets: generate mipmaps only for current frame's device memory
    size_t startIndex = 0;
    size_t endIndex = 1;

    if (isStaticTexture && texture->m_TextureDesc.IsMultiBuffer)
    {
        // Static textures with multi-buffer: generate for all buffers
        endIndex = texture->m_ReadHandles.size();
    }
    else if (isRenderTarget && texture->m_TextureDesc.IsMultiBuffer)
    {
        // Render targets with multi-buffer: generate only for current frame
        startIndex = GetCurrentFrame();
        endIndex = startIndex + 1;
    }
    else
    {
        // Single buffer textures: always use index 0
        startIndex = 0;
        endIndex = 1;
    }

    auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
    if (!l_DX12CommandList)
    {
        Log(Error, texture->m_InstanceName, " Invalid command list");
        return false;
    }

    // Set pipeline state based on texture type
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

    // Set descriptor heaps
    ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
    l_DX12CommandList->SetDescriptorHeaps(1, l_heaps);

    uint32_t l_mipLevels = texture->m_TextureDesc.MipLevels;

    // Process mipmap generation for each device memory on compute command list
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

            // Set texel size constants (1.0 / dstSize)
            l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
            l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

            if (texture->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
            {
                dstDepth = std::max(texture->m_TextureDesc.DepthOrArraySize >> (mipLevel + 1), 1u);
                l_DX12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
            }

            // Bind both UAVs - source and destination
            // Source: mipLevel (read from current mip via UAV)
            // Destination: mipLevel + 1 (write to next smaller mip via UAV)
            auto l_srcHandleIndex = texture->GetHandleIndex(deviceMemoryIndex, mipLevel);
            auto l_srcUAV = D3D12_GPU_DESCRIPTOR_HANDLE { texture->m_WriteHandles[l_srcHandleIndex].m_GPUHandle };

            auto l_dstHandleIndex = texture->GetHandleIndex(deviceMemoryIndex, mipLevel + 1);
            auto l_dstUAV = D3D12_GPU_DESCRIPTOR_HANDLE { texture->m_WriteHandles[l_dstHandleIndex].m_GPUHandle };

            l_DX12CommandList->SetComputeRootDescriptorTable(1, l_srcUAV);
            l_DX12CommandList->SetComputeRootDescriptorTable(2, l_dstUAV);

            // Dispatch compute shader
            uint32_t dispatchX = std::max(dstWidth / 8, 1u);
            uint32_t dispatchY = std::max(dstHeight / 8, 1u);
            uint32_t dispatchZ = std::max(dstDepth / 8, 1u);

            l_DX12CommandList->Dispatch(dispatchX, dispatchY, dispatchZ);
    
            // UAV barrier between mip levels to ensure writes complete
            auto l_uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_defaultHeapBuffer);
            l_DX12CommandList->ResourceBarrier(1, &l_uavBarrier);
        }
    }

    auto memoryCount = endIndex - startIndex;
    Log(Verbose, texture->m_InstanceName, " Successfully recorded mipmap generation commands for ", l_mipLevels, " mip levels for ", memoryCount, " device memory/memories");
    return true;
}

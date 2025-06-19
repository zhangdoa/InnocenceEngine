#include "DX12RenderingServer.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

#ifdef max
#undef max
#endif

#include "../../Engine.h"

#include "DX12Helper_Common.h"

using namespace Inno;
using namespace DX12Helper;

std::optional<uint32_t> DX12RenderingServer::GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility)
{
    if (!rhs)
        return std::nullopt;
    
    if (rhs->m_ObjectStatus != ObjectStatus::Activated)
        return std::nullopt;

    // Use proper handle index based on IsMultiBuffer flag
    auto l_handleIndex = rhs->m_TextureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;
    
    if (bindingAccessibility == Accessibility::ReadOnly)
    {
        if (l_handleIndex < rhs->m_ReadHandles.size())
            return rhs->m_ReadHandles[l_handleIndex].m_Index;
    }
    else if (bindingAccessibility.CanWrite())
    {
        if (l_handleIndex < rhs->m_WriteHandles.size())
            return rhs->m_WriteHandles[l_handleIndex].m_Index;
    }

    return std::nullopt;
}

Vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y)
{
    return Vec4();
}

std::vector<Vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
//     // @TODO: Support different pixel data type
//     auto l_rhs = reinterpret_cast<DX12TextureComponent*>(TextureComp);
    
//     // Use proper device memory index based on IsMultiBuffer flag
//     auto l_deviceMemoryIndex = l_rhs->m_TextureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;
    
//     // Bounds check
//     if (l_deviceMemoryIndex >= l_rhs->m_DeviceMemories.size())
//     {
//         Log(Error, l_rhs, " device memory index ", l_deviceMemoryIndex, " out of bounds (size: ", l_rhs->m_DeviceMemories.size(), ")");
//         return {};
//     }
    
//     auto l_DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_rhs->m_DeviceMemories[l_deviceMemoryIndex]);
//     if (!l_DeviceMemory)
//     {
//         Log(Error, l_rhs, " does not have a valid device memory at index ", l_deviceMemoryIndex);
//         return {};
//     }
    
//     auto l_srcDesc = l_DeviceMemory->m_DefaultHeapBuffer->GetDesc();

//     std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints;
//     l_footprints.resize(l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : l_rhs->m_TextureDesc.DepthOrArraySize);

//     GetDevice()->GetCopyableFootprints(&l_srcDesc, 0, (UINT)l_footprints.size(), 0, l_footprints.data(), NULL, NULL, NULL);

//     if (!l_DeviceMemory->m_ReadBackHeapBuffer)
//     {
//         UINT64 bufferSize = 0;
//         for (size_t i = 0; i < l_footprints.size(); ++i)
//         {
//             bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;
//         }

//         l_DeviceMemory->m_ReadBackHeapBuffer = CreateReadBackHeapBuffer(bufferSize);
// #if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
//         SetObjectName(l_rhs, l_DeviceMemory->m_ReadBackHeapBuffer, "ReadBackHeap_Texture");
// #endif // INNO_DEBUG
//     }

//     auto f_DefaultToReadbackHeap = [this](ComPtr<ID3D12Resource> l_defaultHeapBuffer, ComPtr<ID3D12Resource> l_readbackHeapBuffer, const std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>& footprints, DXGI_FORMAT l_format, D3D12_RESOURCE_STATES currentState)
//         {
//             {
//                 auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

//                 l_commandList->ResourceBarrier(
//                     1,
//                     &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
//                         currentState,
//                         D3D12_RESOURCE_STATE_COMMON));
//                 ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
//             }

//             for (size_t i = 0; i < footprints.size(); i++)
//             {
//                 D3D12_TEXTURE_COPY_LOCATION l_srcLocation = {};
//                 l_srcLocation.pResource = l_defaultHeapBuffer.Get();
//                 l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
//                 l_srcLocation.SubresourceIndex = (UINT)i;

//                 D3D12_TEXTURE_COPY_LOCATION l_destLocation = {};
//                 l_destLocation.pResource = l_readbackHeapBuffer.Get();
//                 l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
//                 l_destLocation.PlacedFootprint = footprints[i];
//                 l_destLocation.PlacedFootprint.Footprint.Format = l_format;

//                 auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_COPY, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY));

//                 l_commandList->ResourceBarrier(
//                     1,
//                     &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
//                         D3D12_RESOURCE_STATE_COMMON,
//                         D3D12_RESOURCE_STATE_COPY_SOURCE));

//                 l_commandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);

//                 l_commandList->ResourceBarrier(
//                     1,
//                     &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
//                         D3D12_RESOURCE_STATE_COPY_SOURCE,
//                         D3D12_RESOURCE_STATE_COMMON));

//                 ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY));
//             }

//             {
//                 auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));
//                 l_commandList->ResourceBarrier(
//                     1,
//                     &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
//                         D3D12_RESOURCE_STATE_COMMON,
//                         currentState));
//                 ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
//             }
//         };

//     size_t l_pixelCount = 0;

//     auto textureDesc = l_rhs->m_TextureDesc;
//     switch (textureDesc.Sampler)
//     {
//     case TextureSampler::Sampler1D:
//         l_pixelCount = textureDesc.Width;
//         break;
//     case TextureSampler::Sampler2D:
//         l_pixelCount = textureDesc.Width * textureDesc.Height;
//         break;
//     case TextureSampler::Sampler3D:
//         l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
//         break;
//     case TextureSampler::Sampler1DArray:
//         l_pixelCount = textureDesc.Width * textureDesc.DepthOrArraySize;
//         break;
//     case TextureSampler::Sampler2DArray:
//         l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
//         break;
//     case TextureSampler::SamplerCubemap:
//         l_pixelCount = textureDesc.Width * textureDesc.Height * 6;
//         break;
//     default:
//         break;
//     }

//     auto f_ReadbackToHostHeap = [](ComPtr<ID3D12Resource> l_readbackHeapBuffer, uint32_t l_pixelDataSize, size_t l_pixelCount) -> std::vector<unsigned char>
//         {
//             std::vector<unsigned char> l_result;
//             l_result.resize(l_pixelCount * l_pixelDataSize);

//             CD3DX12_RANGE m_ReadRange(0, l_result.size());
//             void* l_pData;
//             auto l_HResult = l_readbackHeapBuffer->Map(0, &m_ReadRange, &l_pData);

//             if (FAILED(l_HResult))
//             {
//                 Log(Error, "Can't map texture for CPU to read!");
//             }

//             std::memcpy(l_result.data(), l_pData, l_result.size());
//             l_readbackHeapBuffer->Unmap(0, 0);

//             return l_result;
//         };

//     // Copy from default heap to readback heap, then copy from readback heap to application's heap region
//     DXGI_FORMAT l_format;
    std::vector<Vec4> l_result;

//     if (textureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
//     {
//         l_format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R8_TYPELESS for the stencil
//         f_DefaultToReadbackHeap(l_DeviceMemory->m_DefaultHeapBuffer, l_DeviceMemory->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
//         auto l_rawResult = f_ReadbackToHostHeap(l_DeviceMemory->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);
//         auto l_pixelCount = l_rawResult.size() / 4;

//         std::vector<uint32_t> l_resultUint32;
//         l_resultUint32.resize(l_pixelCount);
//         l_result.resize(l_pixelCount);

//         std::memcpy(l_resultUint32.data(), l_rawResult.data(), l_rawResult.size());
//         for (size_t i = 0; i < l_pixelCount; i++)
//         {
//             auto l_value = l_resultUint32[i];
//             auto l_depth = l_value & 0x00FFFFFF;
//             auto l_stencil = l_value & 0xFF000000;
//             l_result[i].x = float(l_depth) / float(0x00FFFFFF);
//             l_result[i].y = float(l_stencil);
//         }
//     }
//     else
//     {
//         l_format = l_rhs->m_DX12TextureDesc.Format;
//         f_DefaultToReadbackHeap(l_DeviceMemory->m_DefaultHeapBuffer, l_DeviceMemory->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
//         auto l_rawResult = f_ReadbackToHostHeap(l_DeviceMemory->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);

//         l_result.resize(l_pixelCount);
//         if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
//         {
//             for (int i = 0; i < l_pixelCount; ++i)
//             {
//                 const unsigned char* pixelData = &l_rawResult[i * 8];

//                 // Convert the RGBA FLOAT16 data to float values
//                 float floatData[4];
//                 for (int j = 0; j < 4; ++j)
//                 {
//                     const unsigned char* floatBytes = &pixelData[j * 2];
//                     unsigned short float16;
//                     memcpy(&float16, floatBytes, sizeof(unsigned short));
//                     floatData[j] = Math::float16ToFloat32(float16);
//                 }

//                 // Construct a Vec4 from the float values
//                 l_result[i] = Vec4(floatData[0], floatData[1], floatData[2], floatData[3]);
//             }
//         }
//         else
//         {
//             std::memcpy(l_result.data(), l_rawResult.data(), l_rawResult.size());
//         }
//     }

    return l_result;
}

bool DX12RenderingServer::GenerateMipmap(TextureComponent* rhs, ICommandList* commandList)
{
    if (!commandList)
    {
        Log(Error, "GenerateMipmap requires a valid command list for proper synchronization");
        return false;
    }
    
    // Skip SRGB textures for now due to complexity - focus on core functionality
    if(rhs->m_TextureDesc.IsSRGB)
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

    if (rhs->m_TextureDesc.MipLevels == 1)
    {
        Log(Warning, rhs->m_InstanceName, " Attempt to generate mipmaps for texture without mipmaps requirement.");
        return false;
    }

    // Verify texture is in correct state for compute operations
    if (rhs->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        Log(Error, rhs->m_InstanceName, " Texture must be in UNORDERED_ACCESS state for mipmap generation. Current state: ", rhs->m_CurrentState);
        return false;
    }

    // Determine if this is a static texture (Sample) or render target (attachment or compute usage)
    bool isStaticTexture = (rhs->m_TextureDesc.Usage == TextureUsage::Sample);
    bool isRenderTarget = (rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment ||
        rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment ||
        rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment ||
        rhs->m_TextureDesc.Usage == TextureUsage::ComputeOnly);

    // For static textures: generate mipmaps for all device memories
    // For render targets: generate mipmaps only for current frame's device memory
    size_t startIndex = 0;
    size_t endIndex = 1;

    if (isStaticTexture && rhs->m_TextureDesc.IsMultiBuffer)
    {
        // Static textures with multi-buffer: generate for all buffers
        endIndex = rhs->m_ReadHandles.size();
    }
    else if (isRenderTarget && rhs->m_TextureDesc.IsMultiBuffer)
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

    auto l_DX12CommandList = reinterpret_cast<DX12CommandList*>(commandList);
    if (!l_DX12CommandList)
    {
        Log(Error, rhs->m_InstanceName, " Invalid command list");
        return false;
    }

    auto l_computeCommandList = l_DX12CommandList->m_ComputeCommandList;
    if (!l_computeCommandList)
    {
        Log(Error, rhs->m_InstanceName, " Invalid compute command list");
        return false;
    }

    // Set pipeline state based on texture type
    if (rhs->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
    {
        l_computeCommandList->SetComputeRootSignature(m_3DMipmapRootSignature);
        l_computeCommandList->SetPipelineState(m_3DMipmapPSO);
    }
    else
    {
        l_computeCommandList->SetComputeRootSignature(m_2DMipmapRootSignature);
        l_computeCommandList->SetPipelineState(m_2DMipmapPSO);
    }

    // Set descriptor heaps
    ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
    l_computeCommandList->SetDescriptorHeaps(1, l_heaps);

    uint32_t l_mipLevels = rhs->m_TextureDesc.MipLevels;

    // Process mipmap generation for each device memory on compute command list
    for (size_t deviceMemoryIndex = startIndex; deviceMemoryIndex < endIndex; deviceMemoryIndex++)
    {
        auto l_defaultHeapBuffer = reinterpret_cast<ID3D12Resource*>(rhs->m_GPUResources[deviceMemoryIndex]);
        if (!l_defaultHeapBuffer)
        {
            Log(Error, rhs->m_InstanceName, " Invalid device memory at index ", deviceMemoryIndex);
            return false;
        }

        for (uint32_t mipLevel = 0; mipLevel < l_mipLevels - 1; mipLevel++)
        {
            uint32_t dstWidth = std::max(rhs->m_TextureDesc.Width >> (mipLevel + 1), 1u);
            uint32_t dstHeight = std::max(rhs->m_TextureDesc.Height >> (mipLevel + 1), 1u);
            uint32_t dstDepth = 1;

            // Set texel size constants (1.0 / dstSize)
            l_computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
            l_computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

            if (rhs->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
            {
                dstDepth = std::max(rhs->m_TextureDesc.DepthOrArraySize >> (mipLevel + 1), 1u);
                l_computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
            }

            // Bind both UAVs - source and destination
            // Source: mipLevel (read from current mip via UAV)
            // Destination: mipLevel + 1 (write to next smaller mip via UAV)
            auto l_srcHandleIndex = rhs->GetHandleIndex(deviceMemoryIndex, mipLevel);
            auto l_srcUAV = D3D12_GPU_DESCRIPTOR_HANDLE { rhs->m_WriteHandles[l_srcHandleIndex].m_GPUHandle };

            auto l_dstHandleIndex = rhs->GetHandleIndex(deviceMemoryIndex, mipLevel + 1);
            auto l_dstUAV = D3D12_GPU_DESCRIPTOR_HANDLE { rhs->m_WriteHandles[l_dstHandleIndex].m_GPUHandle };

            l_computeCommandList->SetComputeRootDescriptorTable(1, l_srcUAV);
            l_computeCommandList->SetComputeRootDescriptorTable(2, l_dstUAV);

            // Dispatch compute shader
            uint32_t dispatchX = std::max(dstWidth / 8, 1u);
            uint32_t dispatchY = std::max(dstHeight / 8, 1u);
            uint32_t dispatchZ = std::max(dstDepth / 8, 1u);

            l_computeCommandList->Dispatch(dispatchX, dispatchY, dispatchZ);
    
            // UAV barrier between mip levels to ensure writes complete
            auto l_uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_defaultHeapBuffer);
            l_computeCommandList->ResourceBarrier(1, &l_uavBarrier);
        }
    }

    auto memoryCount = endIndex - startIndex;
    Log(Verbose, rhs->m_InstanceName, " Successfully recorded mipmap generation commands for ", l_mipLevels, " mip levels for ", memoryCount, " device memory/memories");
    return true;
}

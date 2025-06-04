#include "DX12RenderingServer.h"
#include "../../Engine.h"

#include "DX12Helper_Common.h"

using namespace Inno;
using namespace DX12Helper;

uint32_t DX12RenderingServer::GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility)
{
    auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);
    if (!l_rhs)
        return 0;
    
    if (l_rhs->m_ObjectStatus != ObjectStatus::Activated)
    {
        Log(Error, "TextureComponent ", l_rhs, " is not activated.");
        return 0;
    }

    auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_rhs->m_DeviceMemories[GetCurrentFrame()]);
    if (!l_DX12DeviceMemory)
    {
        Log(Error, "TextureComponent ", l_rhs, " does not have a valid device memory.");
        return 0;
    }

    if (bindingAccessibility == Accessibility::ReadOnly)
        return l_DX12DeviceMemory->m_SRV.Handle.m_Index;
    else if (bindingAccessibility.CanWrite())
        return l_DX12DeviceMemory->m_UAV.Handle.m_Index;

    return 0;
}

Vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y)
{
    return Vec4();
}

std::vector<Vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
    // @TODO: Support different pixel data type
    auto l_rhs = reinterpret_cast<DX12TextureComponent*>(TextureComp);
    auto l_DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_rhs->m_DeviceMemories[GetCurrentFrame()]);
    auto l_srcDesc = l_DeviceMemory->m_DefaultHeapBuffer->GetDesc();

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints;
    l_footprints.resize(l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : l_rhs->m_TextureDesc.DepthOrArraySize);

    GetDevice()->GetCopyableFootprints(&l_srcDesc, 0, (UINT)l_footprints.size(), 0, l_footprints.data(), NULL, NULL, NULL);

    if (!l_DeviceMemory->m_ReadBackHeapBuffer)
    {
        UINT64 bufferSize = 0;
        for (size_t i = 0; i < l_footprints.size(); ++i)
        {
            bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;
        }

        l_DeviceMemory->m_ReadBackHeapBuffer = CreateReadBackHeapBuffer(bufferSize);
#ifdef INNO_DEBUG
        SetObjectName(l_rhs, l_DeviceMemory->m_ReadBackHeapBuffer, "ReadBackHeap_Texture");
#endif // INNO_DEBUG
    }

    auto f_DefaultToReadbackHeap = [this](ComPtr<ID3D12Resource> l_defaultHeapBuffer, ComPtr<ID3D12Resource> l_readbackHeapBuffer, const std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>& footprints, DXGI_FORMAT l_format, D3D12_RESOURCE_STATES currentState)
        {
            {
                auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

                l_commandList->ResourceBarrier(
                    1,
                    &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
                        currentState,
                        D3D12_RESOURCE_STATE_COMMON));
                ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
            }

            for (size_t i = 0; i < footprints.size(); i++)
            {
                D3D12_TEXTURE_COPY_LOCATION l_srcLocation = {};
                l_srcLocation.pResource = l_defaultHeapBuffer.Get();
                l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                l_srcLocation.SubresourceIndex = (UINT)i;

                D3D12_TEXTURE_COPY_LOCATION l_destLocation = {};
                l_destLocation.pResource = l_readbackHeapBuffer.Get();
                l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                l_destLocation.PlacedFootprint = footprints[i];
                l_destLocation.PlacedFootprint.Footprint.Format = l_format;

                auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_COPY, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY));

                l_commandList->ResourceBarrier(
                    1,
                    &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
                        D3D12_RESOURCE_STATE_COMMON,
                        D3D12_RESOURCE_STATE_COPY_SOURCE));

                l_commandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);

                l_commandList->ResourceBarrier(
                    1,
                    &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
                        D3D12_RESOURCE_STATE_COPY_SOURCE,
                        D3D12_RESOURCE_STATE_COMMON));

                ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY));
            }

            {
                auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));
                l_commandList->ResourceBarrier(
                    1,
                    &CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
                        D3D12_RESOURCE_STATE_COMMON,
                        currentState));
                ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
            }
        };

    size_t l_pixelCount = 0;

    auto textureDesc = l_rhs->m_TextureDesc;
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

    auto f_ReadbackToHostHeap = [](ComPtr<ID3D12Resource> l_readbackHeapBuffer, uint32_t l_pixelDataSize, size_t l_pixelCount) -> std::vector<unsigned char>
        {
            std::vector<unsigned char> l_result;
            l_result.resize(l_pixelCount * l_pixelDataSize);

            CD3DX12_RANGE m_ReadRange(0, l_result.size());
            void* l_pData;
            auto l_HResult = l_readbackHeapBuffer->Map(0, &m_ReadRange, &l_pData);

            if (FAILED(l_HResult))
            {
                Log(Error, "Can't map texture for CPU to read!");
            }

            std::memcpy(l_result.data(), l_pData, l_result.size());
            l_readbackHeapBuffer->Unmap(0, 0);

            return l_result;
        };

    // Copy from default heap to readback heap, then copy from readback heap to application's heap region
    DXGI_FORMAT l_format;
    std::vector<Vec4> l_result;

    if (textureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
    {
        l_format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R8_TYPELESS for the stencil
        f_DefaultToReadbackHeap(l_DeviceMemory->m_DefaultHeapBuffer, l_DeviceMemory->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
        auto l_rawResult = f_ReadbackToHostHeap(l_DeviceMemory->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);
        auto l_pixelCount = l_rawResult.size() / 4;

        std::vector<uint32_t> l_resultUint32;
        l_resultUint32.resize(l_pixelCount);
        l_result.resize(l_pixelCount);

        std::memcpy(l_resultUint32.data(), l_rawResult.data(), l_rawResult.size());
        for (size_t i = 0; i < l_pixelCount; i++)
        {
            auto l_value = l_resultUint32[i];
            auto l_depth = l_value & 0x00FFFFFF;
            auto l_stencil = l_value & 0xFF000000;
            l_result[i].x = float(l_depth) / float(0x00FFFFFF);
            l_result[i].y = float(l_stencil);
        }
    }
    else
    {
        l_format = l_rhs->m_DX12TextureDesc.Format;
        f_DefaultToReadbackHeap(l_DeviceMemory->m_DefaultHeapBuffer, l_DeviceMemory->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
        auto l_rawResult = f_ReadbackToHostHeap(l_DeviceMemory->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);

        l_result.resize(l_pixelCount);
        if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
        {
            for (int i = 0; i < l_pixelCount; ++i)
            {
                const unsigned char* pixelData = &l_rawResult[i * 8];

                // Convert the RGBA FLOAT16 data to float values
                float floatData[4];
                for (int j = 0; j < 4; ++j)
                {
                    const unsigned char* floatBytes = &pixelData[j * 2];
                    unsigned short float16;
                    memcpy(&float16, floatBytes, sizeof(unsigned short));
                    floatData[j] = Math::float16ToFloat32(float16);
                }

                // Construct a Vec4 from the float values
                l_result[i] = Vec4(floatData[0], floatData[1], floatData[2], floatData[3]);
            }
        }
        else
        {
            std::memcpy(l_result.data(), l_rawResult.data(), l_rawResult.size());
        }
    }

    return l_result;
}

// @TODO: This is expensive, it should be running entirely on the GPU
bool DX12RenderingServer::GenerateMipmap(TextureComponent* rhs, ICommandList* commandList)
{
    auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);
    
    // Skip SRGB textures for now due to complexity - focus on core functionality
    if(l_rhs->m_TextureDesc.IsSRGB)
    {
        Log(Warning, "SRGB mipmap generation not currently supported, skipping texture");
        return true;
    }
    
    // If no command list provided, create temporary ones (slow path for runtime)
    if (!commandList)
    {
        DX12CommandList l_tempCommandList = {};
        l_tempCommandList.m_DirectCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));
        l_tempCommandList.m_ComputeCommandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE));
        
        bool result = GenerateMipmapImpl(l_rhs, &l_tempCommandList);
        
        // Execute and wait for completion
        ExecuteCommandListAndWait(l_tempCommandList.m_DirectCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
        ExecuteCommandListAndWait(l_tempCommandList.m_ComputeCommandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE));
        
        return result;
    }
    
    return GenerateMipmapImpl(l_rhs, commandList);
}

#include "DX12TextureResourceService.h"
#include "../../Common/Array.h"
#include "DX12Context.h"
#include "DX12Helper_Texture.h"
#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/MathHelper.h"
#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

Inno::Array<Vec4> DX12TextureResourceService::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
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
    Inno::Array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints(l_subresourceCount);
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
    Inno::Array<unsigned char> l_rawResult(l_bufferSize);

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
    Inno::Array<Vec4> l_result(l_pixelCount);
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

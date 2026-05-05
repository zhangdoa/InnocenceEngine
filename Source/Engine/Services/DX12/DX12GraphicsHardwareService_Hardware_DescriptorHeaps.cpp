#include "DX12GraphicsHardwareService.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_BindlessMesh.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Common/LogService.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12GraphicsHardwareService::CreateGlobalDescriptorHeaps()
{
    auto l_renderingCapacity = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

    uint32_t l_maxGPUBuffers = l_renderingCapacity.maxBuffers;
    uint32_t l_maxMaterialTextures = l_renderingCapacity.maxTextures;
    uint32_t l_maxRenderTargetTextures = 2048;

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers * 3 + l_maxMaterialTextures * 2 + l_maxRenderTargetTextures * 2 + l_renderingCapacity.maxMeshes * 2;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, true);
        m_DX12Context.m_CSUDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderVisible");
        auto l_incrementSize = m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;
        auto l_RenderTargetTextureDescriptorSectionSize = l_maxRenderTargetTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferCBVHandle = {};
        DescriptorHandle l_firstGPUBufferSRVHandle = {};
        DescriptorHandle l_firstGPUBufferUAVHandle = {};

        {
            l_firstGPUBufferCBVHandle.m_CPUHandle = m_DX12Context.m_CSUDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
            l_firstGPUBufferCBVHandle.m_GPUHandle = m_DX12Context.m_CSUDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

            m_DX12Context.m_GPUBuffer_CBV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferCBVHandle, true, L"GPUBuffer_CBV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferCBVHandle;
            D3D12_CONSTANT_BUFFER_VIEW_DESC l_CBVDesc = {};
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateConstantBufferView(nullptr, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstGPUBufferSRVHandle.m_CPUHandle = l_firstGPUBufferCBVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferSRVHandle.m_GPUHandle = l_firstGPUBufferCBVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_GPUBuffer_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferSRVHandle, true, L"GPUBuffer_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            l_SRVDesc.Format = DXGI_FORMAT_R32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstGPUBufferUAVHandle.m_CPUHandle = l_firstGPUBufferSRVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferUAVHandle.m_GPUHandle = l_firstGPUBufferSRVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, true, L"GPUBuffer_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_SINT;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DescriptorHandle l_firstMaterialTextureSRVHandle = {};
        DescriptorHandle l_firstMaterialTextureUAVHandle = {};
        {
            l_firstMaterialTextureSRVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstMaterialTextureSRVHandle.m_GPUHandle = l_firstGPUBufferUAVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_MaterialTexture_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureSRVHandle, true, L"MaterialTexture_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstMaterialTextureUAVHandle.m_CPUHandle = l_firstMaterialTextureSRVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstMaterialTextureUAVHandle.m_GPUHandle = l_firstMaterialTextureSRVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureUAVHandle, true, L"MaterialTexture_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DescriptorHandle l_firstRenderTargetTextureSRVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureUAVHandle = {};
        {
            l_firstRenderTargetTextureSRVHandle.m_CPUHandle = l_firstMaterialTextureUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstRenderTargetTextureSRVHandle.m_GPUHandle = l_firstMaterialTextureUAVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_DX12Context.m_RenderTarget_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureSRVHandle, true, L"RenderTarget_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstRenderTargetTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstRenderTargetTextureUAVHandle.m_CPUHandle = l_firstRenderTargetTextureSRVHandle.m_CPUHandle + l_RenderTargetTextureDescriptorSectionSize;
            l_firstRenderTargetTextureUAVHandle.m_GPUHandle = l_firstRenderTargetTextureSRVHandle.m_GPUHandle + l_RenderTargetTextureDescriptorSectionSize;

            m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureUAVHandle, true, L"RenderTarget_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstRenderTargetTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DX12Helper_BindlessMesh::InitSRVHeapSections(m_DX12Context, l_Desc, l_renderingCapacity.maxMeshes, l_incrementSize, DescriptorHandle{ l_firstRenderTargetTextureUAVHandle.m_CPUHandle + l_RenderTargetTextureDescriptorSectionSize, l_firstRenderTargetTextureUAVHandle.m_GPUHandle + l_RenderTargetTextureDescriptorSectionSize });
    }

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers + l_maxMaterialTextures + l_maxRenderTargetTextures;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, false);
        m_DX12Context.m_CSUDescHeap_ShaderNonVisible = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderNonVisible");

        auto l_incrementSize = m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferUAVHandle = {};
        DescriptorHandle l_firstMaterialTextureBufferUAVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureBufferUAVHandle = {};

        l_firstGPUBufferUAVHandle.m_CPUHandle = m_DX12Context.m_CSUDescHeap_ShaderNonVisible->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstMaterialTextureBufferUAVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
        l_firstRenderTargetTextureBufferUAVHandle.m_CPUHandle = l_firstMaterialTextureBufferUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;

        {
            m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, false, L"GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_UINT;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxMaterialTextures
                , l_incrementSize, l_firstMaterialTextureBufferUAVHandle, false, L"MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstMaterialTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxRenderTargetTextures
                , l_incrementSize, l_firstRenderTargetTextureBufferUAVHandle, false, L"RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstRenderTargetTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }
    }

    {
        uint32_t l_maxSamplerCount = 128;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, l_maxSamplerCount, true);
        m_DX12Context.m_SamplerDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalSamplerDescHeap");

        DescriptorHandle l_firstSamplerHandle = {};
        l_firstSamplerHandle.m_CPUHandle = m_DX12Context.m_SamplerDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstSamplerHandle.m_GPUHandle = m_DX12Context.m_SamplerDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_SamplerDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_SamplerDescHeap, l_Desc, l_maxSamplerCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER), l_firstSamplerHandle, true, L"SamplerDescHeapAccessor");
    }

    {
        uint32_t l_maxRTVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, l_maxRTVCount, false);
        m_DX12Context.m_RTVDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalRTVDescHeap");

        DescriptorHandle l_firstRTVHandle = {};
        l_firstRTVHandle.m_CPUHandle = m_DX12Context.m_RTVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_RTVDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_RTVDescHeap, l_Desc, l_maxRTVCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), l_firstRTVHandle, false, L"RTVDescHeapAccessor");
    }

    {
        uint32_t l_maxDSVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, l_maxDSVCount, false);
        m_DX12Context.m_DSVDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalDSVDescHeap");

        DescriptorHandle l_firstDSVHandle = {};
        l_firstDSVHandle.m_CPUHandle = m_DX12Context.m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_DSVDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_DSVDescHeap, l_Desc, l_maxDSVCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), l_firstDSVHandle, false, L"DSVDescHeapAccessor");
    }

    return true;
}

#include "DX12TextureResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "../FrameManagementService.h"
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

    auto l_DX12CommandList = DX12Helper::AsDX12CommandList(commandList);
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

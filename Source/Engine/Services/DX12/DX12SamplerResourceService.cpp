#include "DX12SamplerResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Texture.h"
#include "../FrameManagementService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12SamplerResourceService::InitializeImpl(SamplerComponent* sampler)
{
	sampler->m_GPUResourceType = GPUResourceType::Sampler;

	D3D12_SAMPLER_DESC l_samplerDesc = {};
	l_samplerDesc.Filter = GetFilterMode(sampler->m_SamplerDesc.m_MinFilterMethod, sampler->m_SamplerDesc.m_MagFilterMethod);
	l_samplerDesc.AddressU = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodU);
	l_samplerDesc.AddressV = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodV);
	l_samplerDesc.AddressW = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodW);
	l_samplerDesc.MipLODBias = 0.0f;
	l_samplerDesc.MaxAnisotropy = sampler->m_SamplerDesc.m_MaxAnisotropy;
	l_samplerDesc.BorderColor[0] = sampler->m_SamplerDesc.m_BorderColor[0];
	l_samplerDesc.BorderColor[1] = sampler->m_SamplerDesc.m_BorderColor[1];
	l_samplerDesc.BorderColor[2] = sampler->m_SamplerDesc.m_BorderColor[2];
	l_samplerDesc.BorderColor[3] = sampler->m_SamplerDesc.m_BorderColor[3];
	l_samplerDesc.MinLOD = sampler->m_SamplerDesc.m_MinLOD;
	l_samplerDesc.MaxLOD = sampler->m_SamplerDesc.m_MaxLOD;

	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
	sampler->m_ReadHandles.resize(l_swapChainImageCount);
	for (auto& handle : sampler->m_ReadHandles)
	{
		handle = m_ctx->m_SamplerDescHeapAccessor.GetNewHandle();
		m_ctx->m_device->CreateSampler(&l_samplerDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ handle.m_CPUHandle });
	}

	sampler->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

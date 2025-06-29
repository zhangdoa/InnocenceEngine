#include "VXGIMultiBouncePass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "VXGIRenderer.h"
#include "VXGIConvertPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;




bool VXGIMultiBouncePass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_VXGIRenderingConfig = &reinterpret_cast<VXGIRendererSystemConfig*>(systemConfig)->m_VXGIRenderingConfig;

	m_TextureComp = l_renderingServer->AddTextureComponent("VoxelMultiBounceVolume/");
	m_TextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_TextureComp->m_TextureDesc.Width = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.Height = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.DepthOrArraySize = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.Usage = TextureUsage::Sample;
	m_TextureComp->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_TextureComp->m_TextureDesc.UseMipMap = true;

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("VoxelMultiBouncePass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "voxelMultiBouncePass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("VoxelMultiBouncePass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 9;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("VoxelMultiBouncePass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIMultiBouncePass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_SamplerComp);
	l_renderingServer->Initialize(m_TextureComp);
	
	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIMultiBouncePass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIMultiBouncePass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIMultiBouncePass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	auto l_renderingContext = reinterpret_cast<VXGIMultiBouncePassRenderingContext*>(renderingContext);
	auto l_VXGIRenderingConfig = reinterpret_cast<VXGIRenderingConfig*>(l_renderingContext);
	auto l_numThreadGroup = l_VXGIRenderingConfig->m_voxelizationResolution / 8;

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 4);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIConvertPass::Get().GetNormalVolume(), 1);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 3);

	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_output, 2);

	// l_renderingServer->Dispatch(m_RenderPassComp, l_numThreadGroup, l_numThreadGroup, l_numThreadGroup);

	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_output, 2);

	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIConvertPass::Get().GetNormalVolume(), 1);

	// l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* VXGIMultiBouncePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* VXGIMultiBouncePass::GetResult()
{
	return m_TextureComp;
}
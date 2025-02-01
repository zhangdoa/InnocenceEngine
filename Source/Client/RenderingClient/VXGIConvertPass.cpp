#include "VXGIConvertPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "VXGIRenderer.h"

#include "../../Engine/Engine.h"

using namespace Inno;




bool VXGIConvertPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_VXGIRenderingConfig = &reinterpret_cast<VXGIRendererSystemConfig*>(systemConfig)->m_VXGIRenderingConfig;
	
	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_AlbedoVolume = l_renderingServer->AddTextureComponent("VoxelAlbedoVolume/");
	m_AlbedoVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_AlbedoVolume->m_TextureDesc.Width = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_AlbedoVolume->m_TextureDesc.Height = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_AlbedoVolume->m_TextureDesc.DepthOrArraySize = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_AlbedoVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_AlbedoVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_AlbedoVolume->m_TextureDesc.UseMipMap = true;

	m_NormalVolume = l_renderingServer->AddTextureComponent("VoxelNormalVolume/");
	m_NormalVolume->m_TextureDesc = m_AlbedoVolume->m_TextureDesc;

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("VoxelConvertPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "voxelConvertPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("VoxelConvertPass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(4);

	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 9;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIConvertPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeTextureComponent(m_AlbedoVolume);
	l_renderingServer->InitializeTextureComponent(m_NormalVolume);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIConvertPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIConvertPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIConvertPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_renderingContext = reinterpret_cast<VXGIConvertPassRenderingContext*>(renderingContext);
	auto l_numThreadGroup = l_renderingContext->m_resolution / 8;

	// l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_AlbedoVolume, 1);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_NormalVolume, 2);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 3);

	// l_renderingServer->Dispatch(m_RenderPassComp,l_numThreadGroup, l_numThreadGroup, l_numThreadGroup);

	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_AlbedoVolume, 1);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_NormalVolume, 2);

	// l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* VXGIConvertPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent * VXGIConvertPass::GetAlbedoVolume()
{
	return m_AlbedoVolume;
}

GPUResourceComponent * VXGIConvertPass::GetNormalVolume()
{
	return m_NormalVolume;
}
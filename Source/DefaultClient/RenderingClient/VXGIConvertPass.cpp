#include "VXGIConvertPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"

#include "VXGIRenderer.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/IGraphicsService.h"

using namespace Inno;




bool VXGIConvertPass::Setup(IServiceConfig *systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	auto l_VXGIRenderingConfig = &reinterpret_cast<VXGIRendererSystemConfig*>(systemConfig)->m_VXGIRenderingConfig;
	
	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_AlbedoVolume = l_graphicsService->AddTextureComponent("VoxelAlbedoVolume/");
	m_AlbedoVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_AlbedoVolume->m_TextureDesc.Width = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_AlbedoVolume->m_TextureDesc.Height = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_AlbedoVolume->m_TextureDesc.DepthOrArraySize = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_AlbedoVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_AlbedoVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_AlbedoVolume->m_TextureDesc.MipLevels = 4;

	m_NormalVolume = l_graphicsService->AddTextureComponent("VoxelNormalVolume/");
	m_NormalVolume->m_TextureDesc = m_AlbedoVolume->m_TextureDesc;

	m_ShaderProgramComp = l_graphicsService->AddShaderProgramComponent("VoxelConvertPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "voxelConvertPass.comp/";

	m_RenderPassComp = l_graphicsService->AddRenderPassComponent("VoxelConvertPass/");

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
	auto l_graphicsService = g_Engine->getGraphicsService();
	
	l_graphicsService->Initialize(m_ShaderProgramComp);
	l_graphicsService->Initialize(m_RenderPassComp);
	l_graphicsService->Initialize(m_AlbedoVolume);
	l_graphicsService->Initialize(m_NormalVolume);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIConvertPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIConvertPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIConvertPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	auto l_renderingContext = reinterpret_cast<VXGIConvertPassRenderingContext*>(renderingContext);
	auto l_numThreadGroup = l_renderingContext->m_resolution / 8;

	// l_graphicsService->CommandListBegin(m_RenderPassComp, 0);
	// l_graphicsService->BindRenderPassComponent(m_RenderPassComp);
	// l_graphicsService->ClearRenderTargets(m_RenderPassComp);

	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_AlbedoVolume, 1);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_NormalVolume, 2);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 3);

	// l_graphicsService->Dispatch(m_RenderPassComp,l_numThreadGroup, l_numThreadGroup, l_numThreadGroup);

	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_AlbedoVolume, 1);
	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_NormalVolume, 2);

	// l_graphicsService->CommandListEnd(m_RenderPassComp);

	return false;
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
#include "VXGIConvertPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VXGIRenderer.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool VXGIConvertPass::Setup(ISystemConfig *systemConfig)
{
	auto l_VXGIRenderingConfig = &reinterpret_cast<VXGIRendererSystemConfig*>(systemConfig)->m_VXGIRenderingConfig;
	
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	m_luminanceVolume = g_Engine->getRenderingServer()->AddTextureComponent("VoxelLuminanceVolume/");
	m_luminanceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_luminanceVolume->m_TextureDesc.Width = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_luminanceVolume->m_TextureDesc.Height = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_luminanceVolume->m_TextureDesc.DepthOrArraySize = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_luminanceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_luminanceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_luminanceVolume->m_TextureDesc.UseMipMap = true;

	m_normalVolume = g_Engine->getRenderingServer()->AddTextureComponent("VoxelNormalVolume/");
	m_normalVolume->m_TextureDesc = m_luminanceVolume->m_TextureDesc;

	m_ShaderProgramComp = g_Engine->getRenderingServer()->AddShaderProgramComponent("VoxelConvertPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "voxelConvertPass.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("VoxelConvertPass/");

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
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_ShaderProgramComp);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_luminanceVolume);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_normalVolume);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIConvertPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIConvertPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIConvertPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingContext = reinterpret_cast<VXGIConvertPassRenderingContext*>(renderingContext);
	auto l_numThreadGroup = l_renderingContext->m_resolution / 8;

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_luminanceVolume, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_normalVolume, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 3, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp,l_numThreadGroup, l_numThreadGroup, l_numThreadGroup);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_luminanceVolume, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_normalVolume, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* VXGIConvertPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent * VXGIConvertPass::GetLuminanceVolume()
{
	return m_luminanceVolume;
}

GPUResourceComponent * VXGIConvertPass::GetNormalVolume()
{
	return m_normalVolume;
}
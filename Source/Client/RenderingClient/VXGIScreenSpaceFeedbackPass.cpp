#include "VXGIScreenSpaceFeedbackPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VXGIRenderer.h"
#include "OpaquePass.h"
#include "LightPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool VXGIScreenSpaceFeedbackPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_VXGIRenderingConfig = &reinterpret_cast<VXGIRendererSystemConfig*>(systemConfig)->m_VXGIRenderingConfig;

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("VoxelScreenSpaceFeedbackPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "voxelScreenSpaceFeedBackPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("VoxelScreenSpaceFeedbackPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	m_TextureComp = l_renderingServer->AddTextureComponent("VoxelScreenSpaceFeedbackVolume/");
	m_TextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_TextureComp->m_TextureDesc.Width = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.Height = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.DepthOrArraySize = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.Usage = TextureUsage::Sample;
	m_TextureComp->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_TextureComp->m_TextureDesc.UseMipMap = true;

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 9;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIScreenSpaceFeedbackPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeTextureComponent(m_TextureComp);
	
	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIScreenSpaceFeedbackPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIScreenSpaceFeedbackPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIScreenSpaceFeedbackPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_renderingContext = reinterpret_cast<VXGIScreenSpaceFeedbackPassRenderingContext*>(renderingContext);
	auto l_viewportSize = g_Engine->getRenderingFrontend()->GetScreenResolution();
	auto l_perFrameGPUBufferComp = DefaultGPUBuffers::GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_perFrameGPUBufferComp, 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightPass::Get().GetIlluminanceResult(), 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_output, 2);

	l_renderingServer->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture, 0);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightPass::Get().GetIlluminanceResult(), 1);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_output, 2);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* VXGIScreenSpaceFeedbackPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* VXGIScreenSpaceFeedbackPass::GetResult()
{
	return m_TextureComp;
}
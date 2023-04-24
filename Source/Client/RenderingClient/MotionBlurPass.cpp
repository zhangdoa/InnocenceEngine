#include "MotionBlurPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool MotionBlurPass::Setup(ISystemConfig *systemConfig)
{
	m_ShaderProgramComp = g_Engine->getRenderingServer()->AddShaderProgramComponent("MotionBlurPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "motionBlurPass.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("MotionBlurPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("MotionBlurPass/");
	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Border;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Border;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool MotionBlurPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_ShaderProgramComp);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool MotionBlurPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus MotionBlurPass::GetStatus()
{
	return m_ObjectStatus;
}

bool MotionBlurPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingContext = reinterpret_cast<MotionBlurPassRenderingContext*>(renderingContext);	
	auto l_viewportSize = g_Engine->getRenderingFrontend()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->ClearRenderTargets(m_RenderPassComp);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 3);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 4, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3].m_Texture, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 1);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0].m_Texture, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3].m_Texture, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 1);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0].m_Texture, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* MotionBlurPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* MotionBlurPass::GetResult()
{
	return m_RenderPassComp->m_RenderTargets[0].m_Texture;
}
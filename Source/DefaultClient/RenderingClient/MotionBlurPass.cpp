#include "MotionBlurPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "OpaquePass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/GraphicsResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;




bool MotionBlurPass::Setup(IServiceConfig *systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = l_rsService->AddShaderProgramComponent("MotionBlurPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "motionBlurPass.comp/";

	m_RenderPassComp = l_rsService->AddRenderPassComponent("MotionBlurPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

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

	m_SamplerComp = l_rsService->AddSamplerComponent("MotionBlurPass/");
	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Border;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Border;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool MotionBlurPass::Initialize()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_rsService->Initialize(m_ShaderProgramComp);
	l_rsService->Initialize(m_RenderPassComp);
	l_rsService->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool MotionBlurPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_rsService->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus MotionBlurPass::GetStatus()
{
	return m_ObjectStatus;
}

bool MotionBlurPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_renderingContext = reinterpret_cast<MotionBlurPassRenderingContext*>(renderingContext);	
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	// l_graphicsService->CommandListBegin(m_RenderPassComp, 0);
	// l_graphicsService->BindRenderPassComponent(m_RenderPassComp);
	// l_graphicsService->ClearRenderTargets(m_RenderPassComp);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 3);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 4);

	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3], 0);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 1);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0], 2);

	// l_graphicsService->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3], 0);
	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 1);
	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0], 2);

	// l_graphicsService->CommandListEnd(m_RenderPassComp);

	return false;
}

RenderPassComponent* MotionBlurPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* MotionBlurPass::GetResult()
{
	if (!m_RenderPassComp)
		return false;
	
	if (!m_RenderPassComp->m_OutputMergerTarget)
		return false;

	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_currentFrame = l_fmService->GetCurrentFrame();

	return m_RenderPassComp->m_OutputMergerTarget->m_ColorOutputs[0];
}
#include "SunShadowBlurEvenPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurOddPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/GraphicsResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;




bool SunShadowBlurEvenPass::Setup(IServiceConfig *systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_shadowMapResolution = SunShadowGeometryProcessPass::Get().GetShadowMapResolution();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_Resizable = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;

	m_ShaderProgramComp = l_rsService->AddShaderProgramComponent("SunShadowBlurEvenPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "sunShadowBlurPassEven.comp/";

	m_RenderPassComp = l_rsService->AddRenderPassComponent("SunShadowBlurEvenPass/");

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs = m_RenderPassComp->m_ResourceBindingLayoutDescs;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_numThreads = TVec4<uint32_t>(16, 16, 1, 0);
	m_numThreadGroups = TVec4<uint32_t>(l_shadowMapResolution / 16, l_shadowMapResolution / 16, 1, 0);
	
	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowBlurEvenPass::Initialize()
{	
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	
	l_rsService->Initialize(m_ShaderProgramComp);
	l_rsService->Initialize(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool SunShadowBlurEvenPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_rsService->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowBlurEvenPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowBlurEvenPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_shadowMapResolution = SunShadowGeometryProcessPass::Get().GetShadowMapResolution();	
	auto l_perFrameGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	// l_graphicsService->CommandListBegin(m_RenderPassComp, 0);
	// l_graphicsService->BindRenderPassComponent(m_RenderPassComp);
	// l_graphicsService->ClearRenderTargets(m_RenderPassComp);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_perFrameGPUBufferComp, 0);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowBlurOddPass::Get().GetResult(), 1);
	// l_graphicsService->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0], 2);

	// l_graphicsService->Dispatch(m_RenderPassComp, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowBlurOddPass::Get().GetResult(), 1);
	// l_graphicsService->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0], 2);

	// l_graphicsService->CommandListEnd(m_RenderPassComp);

	return false;
}

RenderPassComponent* SunShadowBlurEvenPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SunShadowBlurEvenPass::GetResult()
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
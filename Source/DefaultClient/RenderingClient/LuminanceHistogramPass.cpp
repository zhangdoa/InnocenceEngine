#include "LuminanceHistogramPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/GraphicsResourceService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

using namespace Inno;

bool LuminanceHistogramPass::Setup(IServiceConfig* systemConfig)
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_ShaderProgramComp = l_rsService->AddShaderProgramComponent("LuminanceHistogramPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "luminanceHistogramPass.comp/";

	m_RenderPassComp = l_rsService->AddRenderPassComponent("LuminanceHistogramPass/");

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// t0 - LuminanceTexture
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;

	// u0 - LuminanceHistogram
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = l_rsService->AddCommandListComponent("LuminanceHistogramPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_CommandListComp_Graphics = l_rsService->AddCommandListComponent("LuminanceHistogramPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_luminanceHistogram = l_rsService->AddGPUBufferComponent("LuminanceHistogramGPUBuffer/");
	m_luminanceHistogram->m_CPUAccessibility = Accessibility::Immutable;
	m_luminanceHistogram->m_GPUAccessibility = Accessibility::ReadWrite;
	m_luminanceHistogram->m_ElementCount = 256;
	m_luminanceHistogram->m_ElementSize = sizeof(uint32_t);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool LuminanceHistogramPass::Initialize()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_rsService->Initialize(m_ShaderProgramComp);
	l_rsService->Initialize(m_RenderPassComp);
	l_rsService->Initialize(m_CommandListComp_Compute);
	l_rsService->Initialize(m_CommandListComp_Graphics);	
	l_rsService->Initialize(m_luminanceHistogram);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LuminanceHistogramPass::Terminate()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_rsService->Delete(m_luminanceHistogram);
	l_rsService->Delete(m_CommandListComp_Compute);
	l_rsService->Delete(m_CommandListComp_Graphics);
	l_rsService->Delete(m_RenderPassComp);
	l_rsService->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LuminanceHistogramPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LuminanceHistogramPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingContext = reinterpret_cast<LuminanceHistogramPassRenderingContext*>(renderingContext);
	if (l_renderingContext->m_input->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_luminanceHistogram->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / 16);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / 16);

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_hwService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_hwService->TryToTransitState(reinterpret_cast<TextureComponent*>(l_renderingContext->m_input), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_hwService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_hwService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_hwService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_renderingContext->m_input, 1);
	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_luminanceHistogram, 2);

	l_hwService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, (uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1);

	l_hwService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* LuminanceHistogramPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* LuminanceHistogramPass::GetResult()
{
	return m_luminanceHistogram;
}
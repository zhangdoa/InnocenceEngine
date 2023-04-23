#include "LuminanceHistogramPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool LuminanceHistogramPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;
	
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("LuminanceHistogramPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "luminanceHistogramPass.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("LuminanceHistogramPass/");

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_luminanceHistogram = g_Engine->getRenderingServer()->AddGPUBufferComponent("LuminanceHistogramGPUBuffer/");
	m_luminanceHistogram->m_CPUAccessibility = Accessibility::Immutable;
	m_luminanceHistogram->m_GPUAccessibility = Accessibility::ReadWrite;
	m_luminanceHistogram->m_ElementCount = 256;
	m_luminanceHistogram->m_ElementSize = sizeof(uint32_t);

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool LuminanceHistogramPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);

	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_luminanceHistogram);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool LuminanceHistogramPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LuminanceHistogramPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LuminanceHistogramPass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_renderingContext = reinterpret_cast<LuminanceHistogramPassRenderingContext*>(renderingContext);	
	auto l_viewportSize = g_Engine->getRenderingFrontend()->GetScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / 16);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / 16);

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_luminanceHistogram, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 2, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, (uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_luminanceHistogram, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

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
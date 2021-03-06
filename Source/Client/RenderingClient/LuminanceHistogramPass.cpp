#include "LuminanceHistogramPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool LuminanceHistogramPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("LuminanceHistogramPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "luminanceHistogramPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("LuminanceHistogramPass/");

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(3);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_luminanceHistogram = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("LuminanceHistogramGPUBuffer/");
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
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_luminanceHistogram);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool LuminanceHistogramPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

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
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / 16);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / 16);

	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_luminanceHistogram, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 2, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, (uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_luminanceHistogram, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* LuminanceHistogramPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* LuminanceHistogramPass::GetResult()
{
	return m_luminanceHistogram;
}
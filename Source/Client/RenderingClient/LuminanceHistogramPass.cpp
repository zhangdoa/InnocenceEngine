#include "LuminanceHistogramPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace LuminanceHistogramPass
{
	bool CalculateHistogram(GPUResourceComponent* input);
	bool CalculateAverage();

	RenderPassDataComponent* m_RPDC_LuminanceHistogram = 0;
	ShaderProgramComponent* m_SPC_LuminanceHistogram = 0;
	RenderPassDataComponent* m_RPDC_LuminanceAverage = 0;
	ShaderProgramComponent* m_SPC_LuminanceAverage = 0;

	GPUBufferDataComponent* m_LuminanceHistogramGBDC = 0;
	GPUBufferDataComponent* m_LuminanceAverageGBDC = 0;
}

bool LuminanceHistogramPass::Setup()
{
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_SPC_LuminanceHistogram = g_Engine->getRenderingServer()->AddShaderProgramComponent("LuminanceHistogramPass/");

	m_SPC_LuminanceHistogram->m_ShaderFilePaths.m_CSPath = "luminanceHistogramPass.comp/";

	m_RPDC_LuminanceHistogram = g_Engine->getRenderingServer()->AddRenderPassDataComponent("LuminanceHistogramPass/");

	m_RPDC_LuminanceHistogram->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs.resize(3);
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC_LuminanceHistogram->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_RPDC_LuminanceHistogram->m_ShaderProgram = m_SPC_LuminanceHistogram;

	////
	m_SPC_LuminanceAverage = g_Engine->getRenderingServer()->AddShaderProgramComponent("LuminanceAveragePass/");

	m_SPC_LuminanceAverage->m_ShaderFilePaths.m_CSPath = "luminanceAveragePass.comp/";

	m_RPDC_LuminanceAverage = g_Engine->getRenderingServer()->AddRenderPassDataComponent("LuminanceAveragePass/");

	m_RPDC_LuminanceAverage->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs.resize(3);
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC_LuminanceAverage->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_RPDC_LuminanceAverage->m_ShaderProgram = m_SPC_LuminanceAverage;

	////
	m_LuminanceHistogramGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("LuminanceHistogramGPUBuffer/");
	m_LuminanceHistogramGBDC->m_CPUAccessibility = Accessibility::Immutable;
	m_LuminanceHistogramGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_LuminanceHistogramGBDC->m_ElementCount = 256;
	m_LuminanceHistogramGBDC->m_ElementSize = sizeof(uint32_t);

	m_LuminanceAverageGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("LuminanceAverageGPUBuffer/");
	m_LuminanceAverageGBDC->m_CPUAccessibility = Accessibility::Immutable;
	m_LuminanceAverageGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_LuminanceAverageGBDC->m_ElementCount = 1;
	m_LuminanceAverageGBDC->m_ElementSize = sizeof(float);

	return true;
}

bool LuminanceHistogramPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_LuminanceHistogram);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_LuminanceHistogram);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_LuminanceAverage);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_LuminanceAverage);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_LuminanceHistogramGBDC);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_LuminanceAverageGBDC);

	return true;
}

bool LuminanceHistogramPass::CalculateHistogram(GPUResourceComponent* input)
{
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / 16);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / 16);

	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC_LuminanceHistogram, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_LuminanceHistogram);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC_LuminanceHistogram);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_LuminanceHistogram, ShaderStage::Compute, input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_LuminanceHistogram, ShaderStage::Compute, m_LuminanceHistogramGBDC, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_LuminanceHistogram, ShaderStage::Compute, l_PerFrameCBufferGBDC, 2, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC_LuminanceHistogram, (uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC_LuminanceHistogram, ShaderStage::Compute, input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC_LuminanceHistogram, ShaderStage::Compute, m_LuminanceHistogramGBDC, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC_LuminanceHistogram);

	return true;
}

bool LuminanceHistogramPass::CalculateAverage()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC_LuminanceAverage, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_LuminanceAverage);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC_LuminanceAverage);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_LuminanceAverage, ShaderStage::Compute, m_LuminanceHistogramGBDC, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_LuminanceAverage, ShaderStage::Compute, m_LuminanceAverageGBDC, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_LuminanceAverage, ShaderStage::Compute, l_PerFrameCBufferGBDC, 2, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC_LuminanceAverage, 1, 1, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC_LuminanceAverage, ShaderStage::Compute, m_LuminanceHistogramGBDC, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC_LuminanceAverage, ShaderStage::Compute, m_LuminanceAverageGBDC, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC_LuminanceAverage);

	return true;
}

bool LuminanceHistogramPass::Render(GPUResourceComponent* input)
{
	CalculateHistogram(input);
	CalculateAverage();

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC_LuminanceHistogram);

	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC_LuminanceHistogram);

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC_LuminanceAverage);

	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC_LuminanceAverage);

	return true;
}

bool LuminanceHistogramPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_LuminanceHistogram);

	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_LuminanceAverage);

	return true;
}

RenderPassDataComponent* LuminanceHistogramPass::GetRPDC()
{
	return m_RPDC_LuminanceHistogram;
}

ShaderProgramComponent* LuminanceHistogramPass::GetSPC()
{
	return m_SPC_LuminanceHistogram;
}

GPUBufferDataComponent* LuminanceHistogramPass::GetAverageLuminance()
{
	return m_LuminanceAverageGBDC;
}
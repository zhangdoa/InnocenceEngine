#include "LuminanceHistogramPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace LuminanceHistogramPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;

	GPUBufferDataComponent* m_LuminanceHistogramGBDC = 0;
}

bool LuminanceHistogramPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("LuminanceHistogramPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "luminanceHistogramPass.comp/";

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("LuminanceHistogramPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_LuminanceHistogramGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("LuminanceHistogramGPUBuffer/");
	m_LuminanceHistogramGBDC->m_CPUAccessibility = Accessibility::Immutable;
	m_LuminanceHistogramGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_LuminanceHistogramGBDC->m_ElementCount = 256;
	m_LuminanceHistogramGBDC->m_ElementSize = sizeof(uint32_t);
	m_LuminanceHistogramGBDC->m_BindingPoint = 0;

	return true;
}

bool LuminanceHistogramPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_LuminanceHistogramGBDC);

	return true;
}

bool LuminanceHistogramPass::PrepareCommandList(IResourceBinder* input)
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, input, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, m_LuminanceHistogramGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_RPDC, 80, 45, 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, m_LuminanceHistogramGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool LuminanceHistogramPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool LuminanceHistogramPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent * LuminanceHistogramPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * LuminanceHistogramPass::GetSPC()
{
	return m_SPC;
}
#include "BRDFLUTPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "LightPass.h"
#include "SkyPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace BRDFLUTPass
{
	RenderPassDataComponent* m_RPDC;
	RenderPassDataComponent* m_RPDC_MS;
	ShaderProgramComponent* m_SPC;
	ShaderProgramComponent* m_SPC_MS;

	TextureDataComponent* m_TDC;
	TextureDataComponent* m_TDC_MS;
}

bool BRDFLUTPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("BRDFLUTPass/");
	m_SPC->m_ShaderFilePaths.m_CSPath = "BRDFLUTPass.comp/";

	m_SPC_MS = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("BRDFLUTMSPass/");
	m_SPC_MS->m_ShaderFilePaths.m_CSPath = "BRDFLUTMSPass.comp/";

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("BRDFLUTPass/");
	m_RPDC_MS = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("BRDFLUTMSPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 512;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 512;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;
	m_RPDC_MS->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(1);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC_MS->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;
	m_RPDC_MS->m_ShaderProgram = m_SPC_MS;

	m_TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("BRDFLUTPass/");
	m_TDC_MS = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("BRDFLUTMSPass/");

	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_TDC_MS->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	return true;
}

bool BRDFLUTPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_MS);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_MS);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_TDC);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_TDC_MS);

	return true;
}

bool BRDFLUTPass::PrepareCommandList()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC->m_ResourceBinder, 0, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_RPDC, 32, 32, 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC->m_ResourceBinder, 0, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	// Multi-scattering LUT
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_MS, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_MS);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_MS);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_MS, ShaderStage::Compute, m_TDC->m_ResourceBinder, 0, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_MS, ShaderStage::Compute, m_TDC_MS->m_ResourceBinder, 1, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_RPDC_MS, 32, 32, 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_MS, ShaderStage::Compute, m_TDC->m_ResourceBinder, 0, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_MS, ShaderStage::Compute, m_TDC_MS->m_ResourceBinder, 1, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_MS);

	return true;
}

bool BRDFLUTPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_MS);

	return true;
}

RenderPassDataComponent* BRDFLUTPass::GetBRDFLUTRPDC()
{
	return m_RPDC;
}

RenderPassDataComponent* BRDFLUTPass::GetBRDFMSLUTRPDC()
{
	return m_RPDC_MS;
}

IResourceBinder* BRDFLUTPass::GetBRDFLUT()
{
	return m_TDC->m_ResourceBinder;
}

IResourceBinder* BRDFLUTPass::GetBRDFMSLUT()
{
	return m_TDC_MS->m_ResourceBinder;
}
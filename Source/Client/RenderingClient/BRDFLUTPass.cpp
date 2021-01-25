#include "BRDFLUTPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "LightPass.h"
#include "SkyPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

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
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("BRDFLUTPass/");
	m_SPC->m_ShaderFilePaths.m_CSPath = "BRDFLUTPass.comp/";

	m_SPC_MS = g_Engine->getRenderingServer()->AddShaderProgramComponent("BRDFLUTMSPass/");
	m_SPC_MS->m_ShaderFilePaths.m_CSPath = "BRDFLUTMSPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("BRDFLUTPass/");
	m_RPDC_MS = g_Engine->getRenderingServer()->AddRenderPassDataComponent("BRDFLUTMSPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 512;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 512;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;
	m_RPDC_MS->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(1);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC_MS->m_ResourceBindingLayoutDescs.resize(2);
	m_RPDC_MS->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC_MS->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_MS->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;
	m_RPDC_MS->m_ShaderProgram = m_SPC_MS;

	m_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("BRDFLUTPass/");
	m_TDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_TDC_MS = g_Engine->getRenderingServer()->AddTextureDataComponent("BRDFLUTMSPass/");
	m_TDC->m_GPUAccessibility = Accessibility::ReadWrite;

	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_TDC_MS->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	return true;
}

bool BRDFLUTPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_MS);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_MS);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC_MS);

	return true;
}

bool BRDFLUTPass::Render()
{
	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, 32, 32, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	// Multi-scattering LUT
	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC_MS, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_MS);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC_MS);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_MS, ShaderStage::Compute, m_TDC, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC_MS, ShaderStage::Compute, m_TDC_MS, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC_MS, 32, 32, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC_MS, ShaderStage::Compute, m_TDC, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC_MS, ShaderStage::Compute, m_TDC_MS, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC_MS);

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC);
	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC);
	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC_MS);
	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC_MS);

	return true;
}

bool BRDFLUTPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_MS);

	return true;
}

GPUResourceComponent* BRDFLUTPass::GetBRDFLUT()
{
	return m_TDC;
}

GPUResourceComponent* BRDFLUTPass::GetBRDFMSLUT()
{
	return m_TDC_MS;
}
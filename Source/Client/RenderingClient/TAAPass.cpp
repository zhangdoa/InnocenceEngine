#include "TAAPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace TAAPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;

	TextureDataComponent* m_oddTDC;
	TextureDataComponent* m_evenTDC;
	bool m_isPassOdd = true;
}

bool TAAPass::Setup()
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("TAAPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "TAAPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("TAAPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_oddTDC = g_Engine->getRenderingServer()->AddTextureDataComponent("TAAPass_Odd/");
	m_evenTDC = g_Engine->getRenderingServer()->AddTextureDataComponent("TAAPass_Even/");

	m_oddTDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_evenTDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	return true;
}

bool TAAPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_oddTDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_evenTDC);

	return true;
}

bool TAAPass::Render(GPUResourceComponent* input)
{
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	TextureDataComponent* l_writeTDC;
	TextureDataComponent* l_ReadTDC;

	if (m_isPassOdd)
	{
		l_ReadTDC = m_oddTDC;
		l_writeTDC = m_evenTDC;

		m_isPassOdd = false;
	}
	else
	{
		l_ReadTDC = m_evenTDC;
		l_writeTDC = m_oddTDC;

		m_isPassOdd = true;
	}

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, input, 0, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_ReadTDC, 1, 1);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargets[3], 2, 2);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_writeTDC, 3, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 4, 0);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, input, 0, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, l_ReadTDC, 1, 1);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargets[3], 2, 2);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, l_writeTDC, 3, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC);
	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool TAAPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent* TAAPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* TAAPass::GetSPC()
{
	return m_SPC;
}

GPUResourceComponent* TAAPass::GetResult()
{
	if (m_isPassOdd)
	{
		return m_evenTDC;
	}
	else
	{
		return m_oddTDC;
	}
}
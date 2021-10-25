#include "PreTAAPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "LightPass.h"
#include "SkyPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool PreTAAPass::Setup(ISystemConfig *systemConfig)
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("PreTAAPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "preTAAPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("PreTAAPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(3);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("PreTAAPass/");
	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool PreTAAPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool PreTAAPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus PreTAAPass::GetStatus()
{
	return m_ObjectStatus;
}

bool PreTAAPass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, LightPass::Get().GetLuminanceResult(), 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, SkyPass::Get().GetResult(), 1);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, LightPass::Get().GetLuminanceResult(), 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, SkyPass::Get().GetResult(), 1);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* PreTAAPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* PreTAAPass::GetResult()
{
	return m_TDC;
}
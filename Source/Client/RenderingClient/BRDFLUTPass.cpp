#include "BRDFLUTPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

using namespace DefaultGPUBuffers;

bool BRDFLUTPass::Setup(ISystemConfig *systemConfig)
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("BRDFLUTPass/");
	m_SPC->m_ShaderFilePaths.m_CSPath = "BRDFLUTPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("BRDFLUTPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 512;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 512;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(1);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("BRDFLUTPass/");
	m_TDC->m_GPUAccessibility = Accessibility::ReadWrite;

	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool BRDFLUTPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool BRDFLUTPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BRDFLUTPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BRDFLUTPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, 32, 32, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent *BRDFLUTPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent *BRDFLUTPass::GetResult()
{
	return m_TDC;
}
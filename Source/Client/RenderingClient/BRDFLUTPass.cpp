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

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("BRDFLUTPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 512;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 512;
	l_RenderPassDesc.m_Resizable = false;
	
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(1);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_TextureComp = g_Engine->getRenderingServer()->AddTextureComponent("BRDFLUTPass_Result/");
	m_TextureComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_TextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_TextureComp->m_TextureDesc.Usage = TextureUsage::Sample;
	
	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool BRDFLUTPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);

	g_Engine->getRenderingServer()->InitializeTextureComponent(m_TextureComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool BRDFLUTPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BRDFLUTPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BRDFLUTPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, 32, 32, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent *BRDFLUTPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *BRDFLUTPass::GetResult()
{
	return m_TextureComp;
}
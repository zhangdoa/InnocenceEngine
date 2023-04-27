#include "BRDFLUTMSPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "BRDFLUTPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

using namespace DefaultGPUBuffers;

bool BRDFLUTMSPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("BRDFLUTMSPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "BRDFLUTMSPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("BRDFLUTMSPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 512;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 512;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::Sample;
	l_RenderPassDesc.m_Resizable = false;
	
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(2);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool BRDFLUTMSPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);

	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResource = BRDFLUTPass::Get().GetResult();
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResource = m_RenderPassComp->m_RenderTargets[0].m_Texture;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool BRDFLUTMSPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BRDFLUTMSPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BRDFLUTMSPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);
	
	l_renderingServer->Dispatch(m_RenderPassComp, 32, 32, 1);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent *BRDFLUTMSPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *BRDFLUTMSPass::GetResult()
{
	return m_RenderPassComp->m_RenderTargets[0].m_Texture;
}
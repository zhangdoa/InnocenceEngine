#include "PostTAAPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool PostTAAPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("PostTAAPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "postTAAPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("PostTAAPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&PostTAAPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(2);

	// t0 - Input
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ColorAttachment;

	// t1 - Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool PostTAAPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool PostTAAPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_Result);	
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus PostTAAPass::GetStatus()
{
	return m_ObjectStatus;
}

bool PostTAAPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_renderingContext = reinterpret_cast<PostTAAPassRenderingContext*>(renderingContext);
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_Result, 1);

	l_renderingServer->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_Result, 1);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* PostTAAPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* PostTAAPass::GetResult()
{
	return m_Result;
}

bool PostTAAPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_Result)
		l_renderingServer->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_Result = l_renderingServer->AddTextureComponent("Post-TAA Pass Result/");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_renderingServer->Initialize(m_Result);

	return true;
}
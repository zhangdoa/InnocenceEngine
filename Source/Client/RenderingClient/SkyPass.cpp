#include "SkyPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool SkyPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("SkyPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "skyPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("SkyPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&SkyPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(2);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// t0 - Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SkyPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SkyPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SkyPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SkyPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_Result, 1);

	l_renderingServer->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SkyPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SkyPass::GetResult()
{
	return m_Result;
}

bool SkyPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_Result)
		l_renderingServer->DeleteTextureComponent(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_Result = l_renderingServer->AddTextureComponent("Sky Pass Result/");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_renderingServer->InitializeTextureComponent(m_Result);

	return true;
}
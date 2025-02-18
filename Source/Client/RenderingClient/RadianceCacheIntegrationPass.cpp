#include "RadianceCacheIntegrationPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "RadianceCacheReprojectionPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool RadianceCacheIntegrationPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("RadianceCacheIntegrationPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "RadianceCacheIntegration.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("RadianceCacheIntegrationPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&RadianceCacheIntegrationPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(2);

	m_ShaderStage = ShaderStage::Compute;

	// t0 - input radiance cache texture in viewport size and octahedral-projected radiance tiles for probes
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// u0 - output integrated radiance cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool RadianceCacheIntegrationPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool RadianceCacheIntegrationPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_Result);	
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus RadianceCacheIntegrationPass::GetStatus()
{
	return m_ObjectStatus;
}

bool RadianceCacheIntegrationPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	// @TODO: Add a resource owner class to own the radiance cache result, this is confusing to read since the raytracing pass is the final one which writes to the result
	auto l_readTexture = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, l_readTexture, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, m_Result, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetWorldProbeGrid(), 2);

	auto dispatch_x = (l_readTexture->m_TextureDesc.Width + TILE_SIZE - 1) / TILE_SIZE;
	auto dispatch_y = (l_readTexture->m_TextureDesc.Height + TILE_SIZE - 1) / TILE_SIZE;

	l_renderingServer->Dispatch(m_RenderPassComp, dispatch_x, dispatch_y, 1);
	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* RadianceCacheIntegrationPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* RadianceCacheIntegrationPass::GetResult()
{
	return m_Result;
}

bool RadianceCacheIntegrationPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_Result)
		l_renderingServer->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_Result = l_renderingServer->AddTextureComponent("Radiance Cache Integration Result/");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	m_Result->m_TextureDesc.Width = (m_Result->m_TextureDesc.Width + TILE_SIZE - 1) / TILE_SIZE;
	m_Result->m_TextureDesc.Height = (m_Result->m_TextureDesc.Height + TILE_SIZE - 1) / TILE_SIZE;

	// The SH coefficient count is stored per tile of SH_TILE_SIZE * SH_TILE_SIZE, and the coefficient order is (0, 0), (1, 0), (0, 1), (1, 1)
	m_Result->m_TextureDesc.Width *= SH_TILE_SIZE;
	m_Result->m_TextureDesc.Height *= SH_TILE_SIZE;

	l_renderingServer->Initialize(m_Result);

	return true;
}
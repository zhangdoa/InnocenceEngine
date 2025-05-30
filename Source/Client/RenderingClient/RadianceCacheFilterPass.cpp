#include "RadianceCacheFilterPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "RadianceCacheRaytracingPass.h"
#include "RadianceCacheReprojectionPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool RadianceCacheFilterPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("RadianceCacheFilterPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "RadianceCacheFilter.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("RadianceCacheFilterPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&RadianceCacheFilterPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	m_ShaderStage = ShaderStage::Compute;

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// t0 - radiance cache input
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	// t1 - probe positions
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = m_ShaderStage;

	// t2 - probe normals
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = m_ShaderStage;

	// u0 - filtered output
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = m_ShaderStage;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool RadianceCacheFilterPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool RadianceCacheFilterPass::Update()
{
	return true;
}

bool RadianceCacheFilterPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus RadianceCacheFilterPass::GetStatus()
{
	return m_ObjectStatus;
}

bool RadianceCacheFilterPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_raytracingResult = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();
	if (l_raytracingResult->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, l_raytracingResult, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbePosition(), 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbeNormal(), 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_ShaderStage, m_FilteredRadianceCache, 4);

	auto dispatch_x = (l_raytracingResult->m_TextureDesc.Width + TILE_SIZE - 1) / TILE_SIZE;
	auto dispatch_y = (l_raytracingResult->m_TextureDesc.Height + TILE_SIZE - 1) / TILE_SIZE;

	l_renderingServer->Dispatch(m_RenderPassComp, dispatch_x, dispatch_y, 1);
	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* RadianceCacheFilterPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* RadianceCacheFilterPass::GetFilteredResult()
{
	return m_FilteredRadianceCache;
}

bool RadianceCacheFilterPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_raytracingResult = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();

	m_FilteredRadianceCache = l_renderingServer->AddTextureComponent("RadianceCacheFilterPass/");
	m_FilteredRadianceCache->m_TextureDesc = l_raytracingResult->m_TextureDesc;

	l_renderingServer->Initialize(m_FilteredRadianceCache);

	return true;
}

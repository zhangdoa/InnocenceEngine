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

	// Setup horizontal pass
	m_HorizontalShaderProgramComp = l_renderingServer->AddShaderProgramComponent("RadianceCacheFilterHorizontalPass/");
	m_HorizontalShaderProgramComp->m_ShaderFilePaths.m_CSPath = "RadianceCacheFilterHorizontal.comp/";

	m_HorizontalRenderPassComp = l_renderingServer->AddRenderPassComponent("RadianceCacheFilterHorizontalPass/");
	
	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&RadianceCacheFilterPass::RenderTargetsCreationFunc, this);

	m_HorizontalRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	m_ShaderStage = ShaderStage::Compute;

	// Horizontal pass resource bindings
	// b0 - PerFrameCBuffer
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// t0 - radiance cache input
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	// t1 - probe positions
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = m_ShaderStage;

	// t2 - probe normals
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = m_ShaderStage;

	// u0 - horizontal filtered output
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_HorizontalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = m_ShaderStage;

	m_HorizontalRenderPassComp->m_ShaderProgram = m_HorizontalShaderProgramComp;

	// Setup vertical pass
	m_VerticalShaderProgramComp = l_renderingServer->AddShaderProgramComponent("RadianceCacheFilterVerticalPass/");
	m_VerticalShaderProgramComp->m_ShaderFilePaths.m_CSPath = "RadianceCacheFilterVertical.comp/";

	m_VerticalRenderPassComp = l_renderingServer->AddRenderPassComponent("RadianceCacheFilterVerticalPass/");
	m_VerticalRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	// Vertical pass resource bindings
	// b0 - PerFrameCBuffer
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// t0 - horizontal filtered input
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	// t1 - probe positions
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = m_ShaderStage;

	// t2 - probe normals
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = m_ShaderStage;

	// u0 - final filtered output
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_VerticalRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = m_ShaderStage;

	m_VerticalRenderPassComp->m_ShaderProgram = m_VerticalShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool RadianceCacheFilterPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_HorizontalShaderProgramComp);
	l_renderingServer->Initialize(m_HorizontalRenderPassComp);
	
	l_renderingServer->Initialize(m_VerticalShaderProgramComp);
	l_renderingServer->Initialize(m_VerticalRenderPassComp);

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

	l_renderingServer->Delete(m_HorizontalRenderPassComp);
	l_renderingServer->Delete(m_HorizontalShaderProgramComp);
	
	l_renderingServer->Delete(m_VerticalRenderPassComp);
	l_renderingServer->Delete(m_VerticalShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus RadianceCacheFilterPass::GetStatus()
{
	return m_ObjectStatus;
}

bool RadianceCacheFilterPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_HorizontalRenderPassComp->m_ObjectStatus != ObjectStatus::Activated ||
		m_VerticalRenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_HorizontalFilteredCache->m_ObjectStatus != ObjectStatus::Activated ||
		m_FilteredRadianceCache->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_raytracingResult = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();
	if (l_raytracingResult->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	// HORIZONTAL PASS
	l_renderingServer->CommandListBegin(m_HorizontalRenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_HorizontalRenderPassComp);

	l_renderingServer->BindGPUResource(m_HorizontalRenderPassComp, m_ShaderStage, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_HorizontalRenderPassComp, m_ShaderStage, l_raytracingResult, 1);
	l_renderingServer->BindGPUResource(m_HorizontalRenderPassComp, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbePosition(), 2);
	l_renderingServer->BindGPUResource(m_HorizontalRenderPassComp, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbeNormal(), 3);
	l_renderingServer->BindGPUResource(m_HorizontalRenderPassComp, m_ShaderStage, m_HorizontalFilteredCache, 4);

	auto dispatch_x = (l_raytracingResult->m_TextureDesc.Width + TILE_SIZE - 1) / TILE_SIZE;
	auto dispatch_y = (l_raytracingResult->m_TextureDesc.Height + TILE_SIZE - 1) / TILE_SIZE;

	l_renderingServer->Dispatch(m_HorizontalRenderPassComp, dispatch_x, dispatch_y, 1);
	l_renderingServer->CommandListEnd(m_HorizontalRenderPassComp);

	// VERTICAL PASS
	l_renderingServer->CommandListBegin(m_VerticalRenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_VerticalRenderPassComp);

	l_renderingServer->BindGPUResource(m_VerticalRenderPassComp, m_ShaderStage, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_VerticalRenderPassComp, m_ShaderStage, m_HorizontalFilteredCache, 1); // Use horizontal output as input
	l_renderingServer->BindGPUResource(m_VerticalRenderPassComp, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbePosition(), 2);
	l_renderingServer->BindGPUResource(m_VerticalRenderPassComp, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbeNormal(), 3);
	l_renderingServer->BindGPUResource(m_VerticalRenderPassComp, m_ShaderStage, m_FilteredRadianceCache, 4);

	l_renderingServer->Dispatch(m_VerticalRenderPassComp, dispatch_x, dispatch_y, 1);
	l_renderingServer->CommandListEnd(m_VerticalRenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* RadianceCacheFilterPass::GetRenderPassComp()
{
	return m_VerticalRenderPassComp; // Return final pass for dependency management
}

RenderPassComponent* RadianceCacheFilterPass::GetHorizontalRenderPassComp()
{
	return m_HorizontalRenderPassComp;
}

RenderPassComponent* RadianceCacheFilterPass::GetVerticalRenderPassComp()
{
	return m_VerticalRenderPassComp;
}

TextureComponent* RadianceCacheFilterPass::GetFilteredResult()
{
	return m_FilteredRadianceCache;
}

bool RadianceCacheFilterPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_raytracingResult = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();

	// Create intermediate texture for horizontal pass output
	m_HorizontalFilteredCache = l_renderingServer->AddTextureComponent("RadianceCacheFilterHorizontalPass/");
	m_HorizontalFilteredCache->m_TextureDesc = l_raytracingResult->m_TextureDesc;
	l_renderingServer->Initialize(m_HorizontalFilteredCache);

	// Create final filtered output texture
	m_FilteredRadianceCache = l_renderingServer->AddTextureComponent("RadianceCacheFilterVerticalPass/");
	m_FilteredRadianceCache->m_TextureDesc = l_raytracingResult->m_TextureDesc;
	l_renderingServer->Initialize(m_FilteredRadianceCache);

	return true;
}

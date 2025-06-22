#include "RadianceCacheFilterHorizontalPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "RadianceCacheReprojectionPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool RadianceCacheFilterHorizontalPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("RadianceCacheFilterHorizontalPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "RadianceCacheFilterHorizontal.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("RadianceCacheFilterHorizontalPass/");
	
	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&RadianceCacheFilterHorizontalPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	// t0 - radiance cache input
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	// t1 - probe positions
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Compute;

	// t2 - probe normals
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Compute;

	// u0 - horizontal filtered output
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = l_renderingServer->AddCommandListComponent("RadianceCacheFilterHorizontalPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = l_renderingServer->AddCommandListComponent("RadianceCacheFilterHorizontalPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool RadianceCacheFilterHorizontalPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_CommandListComp_Graphics);
	l_renderingServer->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool RadianceCacheFilterHorizontalPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_Result);
	l_renderingServer->Delete(m_CommandListComp_Compute);
	l_renderingServer->Delete(m_CommandListComp_Graphics);
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus RadianceCacheFilterHorizontalPass::GetStatus()
{
	return m_ObjectStatus;
}

bool RadianceCacheFilterHorizontalPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_raytracingResult = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();
	if (l_raytracingResult->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	// Use graphics command list to transition resources
	l_renderingServer->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(l_raytracingResult), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(RadianceCacheReprojectionPass::Get().GetCurrentProbePosition()), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(RadianceCacheReprojectionPass::Get().GetCurrentProbeNormal()), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::WriteOnly);
	l_renderingServer->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_renderingServer->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_raytracingResult, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, RadianceCacheReprojectionPass::Get().GetCurrentProbePosition(), 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, RadianceCacheReprojectionPass::Get().GetCurrentProbeNormal(), 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 4);

	auto dispatch_x = (l_raytracingResult->m_TextureDesc.Width + TILE_SIZE - 1) / TILE_SIZE;
	auto dispatch_y = (l_raytracingResult->m_TextureDesc.Height + TILE_SIZE - 1) / TILE_SIZE;

	l_renderingServer->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, dispatch_x, dispatch_y, 1);
	l_renderingServer->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* RadianceCacheFilterHorizontalPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* RadianceCacheFilterHorizontalPass::GetResult()
{
	return m_Result;
}

bool RadianceCacheFilterHorizontalPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_raytracingResult = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();

	if (m_Result)
		l_renderingServer->Delete(m_Result);

	m_Result = l_renderingServer->AddTextureComponent("RadianceCacheFilterHorizontalPass_Result/");
	m_Result->m_TextureDesc = l_raytracingResult->m_TextureDesc;
	l_renderingServer->Initialize(m_Result);

	return true;
}
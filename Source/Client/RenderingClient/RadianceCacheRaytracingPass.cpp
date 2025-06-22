#include "RadianceCacheRaytracingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "OpaquePass.h"
#include "LightPass.h"
#include "RadianceCacheReprojectionPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool RadianceCacheRaytracingPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_SamplerComp = l_renderingServer->AddSamplerComponent("RadianceCacheRaytracingPass/");

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("RadianceCacheRaytracingPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_RayGenPath = "RadianceCacheRayGen.hlsl/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_ClosestHitPath = "RadianceCacheClosestHit.hlsl/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_AnyHitPath = "RadianceCacheAnyHit.hlsl/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_MissPath = "RadianceCacheMiss.hlsl/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("RadianceCacheRaytracingPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseRaytracing = true;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&RadianceCacheRaytracingPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(12);

	m_ShaderStage = ShaderStage::RayGen | ShaderStage::ClosestHit | ShaderStage::AnyHit | ShaderStage::Miss;

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// t0 - TLAS
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUBufferUsage = GPUBufferUsage::TLAS;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	// t1 - world position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = m_ShaderStage;

	// t2 - world normal + metallic
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = m_ShaderStage;

	// t3 - albedo + roughness
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = m_ShaderStage;

	// t4 - motion vector + AO + transparency
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = m_ShaderStage;

	// t5 - light pass illuminance result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage = m_ShaderStage;

	// t6 - previous frame radiance cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 6;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage = m_ShaderStage;

	// u0 - radiance cache from current frame
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_ShaderStage = m_ShaderStage;

	// u1 - world probe grid
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_ShaderStage = m_ShaderStage;

	// u2 - probe position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_TextureUsage = TextureUsage::ColorAttachment;	
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ShaderStage = m_ShaderStage;

	// u3 - probe normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_ShaderStage = m_ShaderStage;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = l_renderingServer->AddCommandListComponent("RadianceCacheRaytracingPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = l_renderingServer->AddCommandListComponent("RadianceCacheRaytracingPass/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool RadianceCacheRaytracingPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_CommandListComp_Graphics);
	l_renderingServer->Initialize(m_CommandListComp_Compute);
	l_renderingServer->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool RadianceCacheRaytracingPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_SamplerComp);	
	l_renderingServer->Delete(m_CommandListComp_Compute);
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus RadianceCacheRaytracingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool RadianceCacheRaytracingPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_result = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();
	if (l_result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	// Use graphics command list to transition resources to shader resource state
	l_renderingServer->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0]), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1]), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[2]), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3]), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(LightPass::Get().GetIlluminanceResult(), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(RadianceCacheReprojectionPass::Get().GetPreviousFrameResult(), m_CommandListComp_Graphics, Accessibility::ReadOnly);
	l_renderingServer->TryToTransitState(l_result, m_CommandListComp_Graphics, Accessibility::WriteOnly);
	l_renderingServer->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_renderingServer->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_renderingServer->GetTLASBuffer(), 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[2], 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3], 5);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, LightPass::Get().GetIlluminanceResult(), 6);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetPreviousFrameResult(), 7);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_result, 8);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetWorldProbeGrid(), 9);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbePosition(), 10);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, RadianceCacheReprojectionPass::Get().GetCurrentProbeNormal(), 11);

	auto dispatch_x = (l_result->m_TextureDesc.Width + TILE_SIZE - 1) / TILE_SIZE;  // Round up
	auto dispatch_y = (l_result->m_TextureDesc.Height + TILE_SIZE - 1) / TILE_SIZE;  // Round up

	l_renderingServer->DispatchRays(m_RenderPassComp, m_CommandListComp_Compute, dispatch_x, dispatch_y, 1);
	l_renderingServer->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* RadianceCacheRaytracingPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool RadianceCacheRaytracingPass::RenderTargetsCreationFunc()
{

	return true;
}
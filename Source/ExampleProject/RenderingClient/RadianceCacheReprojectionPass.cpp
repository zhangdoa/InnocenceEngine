#include "RadianceCacheReprojectionPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "OpaquePass.h"
#include "RadianceCacheRaytracingPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool RadianceCacheReprojectionPass::Setup(IServiceConfig* systemConfig)
{

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("RadianceCacheReprojectionPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "RadianceCacheReprojection.comp/";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("RadianceCacheReprojectionPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&RadianceCacheReprojectionPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(8);

	m_ShaderStage = ShaderStage::Compute;

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// t0 - world position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	// t1 - world normal + metallic
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = m_ShaderStage;

	// t2 - motion vector + AO + transparency
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = m_ShaderStage;

	// t3 - previous frame radiance cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = m_ShaderStage;

	// t4 - previous probe world position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = m_ShaderStage;

	// t5 - previous probe world normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage = m_ShaderStage;

	// u0 - current frame radiance cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage = m_ShaderStage;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("RadianceCacheReprojectionPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("RadianceCacheReprojectionPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool RadianceCacheReprojectionPass::Initialize()
{

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool RadianceCacheReprojectionPass::Terminate()
{

	g_Engine->Get<GPUBufferResourceService>()->Delete(m_WorldProbeGrid);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Odd);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Odd);
	g_Engine->Get<TextureResourceService>()->Delete(m_RadianceCache_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_RadianceCache_Odd);
	
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus RadianceCacheReprojectionPass::GetStatus()
{
	return m_ObjectStatus;
}

bool RadianceCacheReprojectionPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_RadianceCache_Even->m_ObjectStatus != ObjectStatus::Activated
		|| m_RadianceCache_Odd->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_readTexture = GetPreviousFrameResult();
	auto l_writeTexture = GetCurrentFrameResult();
	auto l_probePosition = GetPreviousProbePosition();
	auto l_probeNormal = GetPreviousProbeNormal();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	// Transition OpaquePass RTs to COMMON on the Graphics queue for cross-queue access.
	// D3D12 requires COMMON state for cross-queue sharing; the Compute queue auto-promotes
	// from COMMON to NON_PIXEL_SHADER_RESOURCE via implicit promotion.
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0]), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::CrossQueueTransition);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1]), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::CrossQueueTransition);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3]), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::CrossQueueTransition);
	
	// Transition read textures from their current state to ReadOnly
	l_fmService->TryToTransitState(l_readTexture, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_probePosition, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_probeNormal, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_writeTexture, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	g_Engine->Get<TextureResourceService>()->Clear(m_CommandListComp_Graphics, l_writeTexture);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3], 3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_readTexture, 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_probePosition, 5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_probeNormal, 6);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_writeTexture, 7);

	auto dispatch_x = (l_writeTexture->m_TextureDesc.Width + TILE_SIZE - 1) / TILE_SIZE;
	auto dispatch_y = (l_writeTexture->m_TextureDesc.Height + TILE_SIZE - 1) / TILE_SIZE;

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, dispatch_x, dispatch_y, 1);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;
	

	return true;
}

RenderPassComponent* RadianceCacheReprojectionPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool RadianceCacheReprojectionPass::RenderTargetsCreationFunc()
{

	if (m_RadianceCache_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_RadianceCache_Even);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::ComputeOnly;
	m_RadianceCache_Even = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Result (Even)/");
	m_RadianceCache_Even->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_RadianceCache_Even);

	if (m_RadianceCache_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_RadianceCache_Odd);

	m_RadianceCache_Odd = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Result (Odd)/");
	m_RadianceCache_Odd->m_TextureDesc = m_RadianceCache_Even->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_RadianceCache_Odd);

	if (m_ProbePosition_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Odd);

	auto l_probeTextureWidth = (l_RenderPassDesc.m_RenderTargetDesc.Width + TILE_SIZE - 1) / TILE_SIZE;
	auto l_probeTextureHeight = (l_RenderPassDesc.m_RenderTargetDesc.Height + TILE_SIZE - 1) / TILE_SIZE;
	m_ProbePosition_Odd = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Position (Odd)/");
	m_ProbePosition_Odd->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_ProbePosition_Odd->m_TextureDesc.Width = l_probeTextureWidth;
	m_ProbePosition_Odd->m_TextureDesc.Height = l_probeTextureHeight;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbePosition_Odd);

	if (m_ProbePosition_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Even);

	m_ProbePosition_Even = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Position (Even)/");
	m_ProbePosition_Even->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbePosition_Even);

	if (m_ProbeNormal_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Odd);

	m_ProbeNormal_Odd = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Normal (Odd)/");
	m_ProbeNormal_Odd->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbeNormal_Odd);

	if (m_ProbeNormal_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Even);

	m_ProbeNormal_Even = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Normal (Even)/");
	m_ProbeNormal_Even->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbeNormal_Even);

	if (m_WorldProbeGrid)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_WorldProbeGrid);

	m_WorldProbeGrid = g_Engine->Get<GPUBufferResourceService>()->Add("Radiance Cache World Probe Grid/");
	m_WorldProbeGrid->m_GPUAccessibility = Accessibility::ReadWrite;
	m_WorldProbeGrid->m_ElementCount = 256 * 1024;
	m_WorldProbeGrid->m_ElementSize = sizeof(float) * 3 + sizeof(float) * 3 + sizeof(float);
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_WorldProbeGrid);

	return true;
}

TextureComponent* RadianceCacheReprojectionPass::GetCurrentFrameResult()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_RadianceCache_Odd : m_RadianceCache_Even;
}

TextureComponent* RadianceCacheReprojectionPass::GetPreviousFrameResult()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_RadianceCache_Even : m_RadianceCache_Odd;
}

TextureComponent* RadianceCacheReprojectionPass::GetCurrentProbePosition()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbePosition_Odd : m_ProbePosition_Even;
}

TextureComponent* Inno::RadianceCacheReprojectionPass::GetPreviousProbePosition()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbePosition_Even : m_ProbePosition_Odd;
}

TextureComponent* RadianceCacheReprojectionPass::GetCurrentProbeNormal()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbeNormal_Odd : m_ProbeNormal_Even;
}

TextureComponent* Inno::RadianceCacheReprojectionPass::GetPreviousProbeNormal()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbeNormal_Even : m_ProbeNormal_Odd;
}

GPUBufferComponent* RadianceCacheReprojectionPass::GetWorldProbeGrid()
{
	return m_WorldProbeGrid;
}
#include "SSRCTemporalPass.h"
#include "SSRCConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "OpaquePass.h"
#include "SSRCIntegrationPass.h"
#include "SSRCReprojectionPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool SSRCTemporalPass::Setup(IServiceConfig* systemConfig)
{
	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SSRCTemporalPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "SSRCTemporal.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SSRCTemporalPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&SSRCTemporalPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// Layout: 1 CB + 10 SRVs + 4 UAVs = 15 descs. The SVGF moments
	// ping-pong (was t8 / u1) was retired in TASK-6.2 — the §2.4.3
	// paper-faithful spatial filter no longer reads luminance variance,
	// so the moments texture and its bindings are dead weight.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(15);

	// b0 - PerFrame CBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// t0 - World Position (G-buffer RT0)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;

	// t1 - World Normal (G-buffer RT1)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;

	// t2 - Motion vectors (G-buffer RT3)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;

	// t3 - Radiance cache SH atlas
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;

	// t4 - Probe position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ComputeOnly;

	// t5 - Probe normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ComputeOnly;

	// t6 - Probe mask
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 6;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ComputeOnly;

	// t7 - Previous-frame GI history (rgb = radiance·N, a = N)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 7;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_TextureUsage = TextureUsage::ComputeOnly;

	// t8 - Previous-frame world position (CL1 prev-depth source)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 8;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_TextureUsage = TextureUsage::ComputeOnly;

	// t9 - Previous-frame colour-delta (R16_FLOAT)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 9;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_TextureUsage = TextureUsage::ComputeOnly;

	// u0 - Current-frame GI history (write target)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_TextureUsage = TextureUsage::ComputeOnly;

	// u1 - Per-pixel blur mask (R Float16, Capsaicin gi1.comp:4099)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_TextureUsage = TextureUsage::ComputeOnly;

	// u2 - Current-frame world position (write target — next frame's prev)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_TextureUsage = TextureUsage::ComputeOnly;

	// u3 - Current-frame colour-delta (write target)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_TextureUsage = TextureUsage::ComputeOnly;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("SSRCTemporalPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("SSRCTemporalPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SSRCTemporalPass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SSRCTemporalPass::Terminate()
{
	g_Engine->Get<TextureResourceService>()->Delete(m_GIHistory_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_GIHistory_Odd);
	g_Engine->Get<TextureResourceService>()->Delete(m_PrevWorldPos_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_PrevWorldPos_Odd);
	g_Engine->Get<TextureResourceService>()->Delete(m_ColorDelta_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_ColorDelta_Odd);
	g_Engine->Get<TextureResourceService>()->Delete(m_BlurMask);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SSRCTemporalPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SSRCTemporalPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_GIHistory_Even || m_GIHistory_Even->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_GIHistory_Odd || m_GIHistory_Odd->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_PrevWorldPos_Even || m_PrevWorldPos_Even->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_PrevWorldPos_Odd || m_PrevWorldPos_Odd->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_ColorDelta_Even || m_ColorDelta_Even->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_ColorDelta_Odd || m_ColorDelta_Odd->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (!m_BlurMask || m_BlurMask->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	auto l_previousHistory = GetPreviousResult();
	auto l_currentHistory = GetCurrentResult();
	auto l_previousWorldPos = GetPreviousPrevWorldPos();
	auto l_currentWorldPos = GetCurrentPrevWorldPos();
	auto l_previousColorDelta = GetPreviousColorDelta();
	auto l_currentColorDelta = GetCurrentColorDelta();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(SSRCIntegrationPass::Get().GetResult()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(SSRCReprojectionPass::Get().GetCurrentProbePosition(), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(SSRCReprojectionPass::Get().GetCurrentProbeNormal(), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(SSRCReprojectionPass::Get().GetProbeMask(), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_previousHistory, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_currentHistory, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->TryToTransitState(l_previousWorldPos, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_currentWorldPos, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->TryToTransitState(l_previousColorDelta, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_currentColorDelta, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->TryToTransitState(m_BlurMask, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3], 3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSRCIntegrationPass::Get().GetResult(), 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSRCReprojectionPass::Get().GetCurrentProbePosition(), 5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSRCReprojectionPass::Get().GetCurrentProbeNormal(), 6);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSRCReprojectionPass::Get().GetProbeMask(), 7);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_previousHistory, 8);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_previousWorldPos, 9);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_previousColorDelta, 10);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currentHistory, 11);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_BlurMask, 12);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currentWorldPos, 13);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currentColorDelta, 14);

	// Match the [numthreads(8,8,1)] in SSRCTemporal.comp: one group per
	// SSRC::TILE_SIZE × TILE_SIZE pixel tile, ceiling-divided so a
	// viewport that is not a multiple of the tile size still covers the
	// right and bottom strip. Dropping the ceiling here would mirror the
	// TASK-127 cropping bug from the SH-atlas allocation side.
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute,
		SSRC::TileCount(uint32_t(l_viewportSize.x)),
		SSRC::TileCount(uint32_t(l_viewportSize.y)),
		1);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SSRCTemporalPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* SSRCTemporalPass::GetCurrentResult()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	return (l_frameCount % 2 == 1) ? m_GIHistory_Odd : m_GIHistory_Even;
}

TextureComponent* SSRCTemporalPass::GetPreviousResult()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	return (l_frameCount % 2 == 1) ? m_GIHistory_Even : m_GIHistory_Odd;
}

TextureComponent* SSRCTemporalPass::GetCurrentPrevWorldPos()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	return (l_frameCount % 2 == 1) ? m_PrevWorldPos_Odd : m_PrevWorldPos_Even;
}

TextureComponent* SSRCTemporalPass::GetPreviousPrevWorldPos()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	return (l_frameCount % 2 == 1) ? m_PrevWorldPos_Even : m_PrevWorldPos_Odd;
}

TextureComponent* SSRCTemporalPass::GetCurrentColorDelta()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	return (l_frameCount % 2 == 1) ? m_ColorDelta_Odd : m_ColorDelta_Even;
}

TextureComponent* SSRCTemporalPass::GetPreviousColorDelta()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	return (l_frameCount % 2 == 1) ? m_ColorDelta_Even : m_ColorDelta_Odd;
}

TextureComponent* SSRCTemporalPass::GetBlurMask()
{
	return m_BlurMask;
}

bool SSRCTemporalPass::RenderTargetsCreationFunc()
{
	if (m_GIHistory_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_GIHistory_Even);
	if (m_GIHistory_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_GIHistory_Odd);
	if (m_PrevWorldPos_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_PrevWorldPos_Even);
	if (m_PrevWorldPos_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_PrevWorldPos_Odd);
	if (m_ColorDelta_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_ColorDelta_Even);
	if (m_ColorDelta_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_ColorDelta_Odd);
	if (m_BlurMask)
		g_Engine->Get<TextureResourceService>()->Delete(m_BlurMask);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_GIHistory_Even = g_Engine->Get<TextureResourceService>()->Add("SSRCTemporalPass History (Even)");
	m_GIHistory_Even->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_GIHistory_Even->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_GIHistory_Even);

	m_GIHistory_Odd = g_Engine->Get<TextureResourceService>()->Add("SSRCTemporalPass History (Odd)");
	m_GIHistory_Odd->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_GIHistory_Odd->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_GIHistory_Odd);

	// Prev-world-pos: RGBA Float16 default. xyz = world, w = 1/0 sky
	// flag. Validity gate compares against depth-scaled `cell_size` so
	// Float16 precision loss at distance scales with the gate threshold.
	m_PrevWorldPos_Even = g_Engine->Get<TextureResourceService>()->Add("SSRCTemporalPass PrevWorldPos (Even)");
	m_PrevWorldPos_Even->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_PrevWorldPos_Even->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_PrevWorldPos_Even);

	m_PrevWorldPos_Odd = g_Engine->Get<TextureResourceService>()->Add("SSRCTemporalPass PrevWorldPos (Odd)");
	m_PrevWorldPos_Odd->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_PrevWorldPos_Odd->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_PrevWorldPos_Odd);

	// Colour-delta: scalar R16_FLOAT — luma residual smoothed at 1/8.
	// Float16 covers the practical range (−luma, +luma) at our HDR scale.
	auto l_ColorDeltaDesc = l_RenderPassDesc.m_RenderTargetDesc;
	l_ColorDeltaDesc.PixelDataFormat = TexturePixelDataFormat::R;

	m_ColorDelta_Even = g_Engine->Get<TextureResourceService>()->Add("SSRCTemporalPass ColorDelta (Even)");
	m_ColorDelta_Even->m_TextureDesc = l_ColorDeltaDesc;
	m_ColorDelta_Even->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_ColorDelta_Even);

	m_ColorDelta_Odd = g_Engine->Get<TextureResourceService>()->Add("SSRCTemporalPass ColorDelta (Odd)");
	m_ColorDelta_Odd->m_TextureDesc = l_ColorDeltaDesc;
	m_ColorDelta_Odd->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_ColorDelta_Odd);

	// Per-pixel blur mask. Scalar R Float16, single-buffered — same
	// PixelDataFormat treatment as the colour-delta scalar above. Read
	// then overwritten within the same frame by SSRCSpatial{Horizontal,Vertical}Pass.
	m_BlurMask = g_Engine->Get<TextureResourceService>()->Add("SSRCTemporalPass BlurMask");
	m_BlurMask->m_TextureDesc = l_ColorDeltaDesc;
	m_BlurMask->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_BlurMask);

	return true;
}

#include "PTNRDCompositionPass.h"

#include "GPUPathTracerPass.h"
#include "PTNRDDenoisePass.h"
#include "NRDConstants.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool PTNRDCompositionPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	// Mirror the ExecuteCommands gate: this pass's CLs are only submitted
	// when the pass-level status is Activated (i.e. inputs+output ready).
	// Recording barriers when the CL won't be submitted desynchronises the
	// engine's per-resource state tracker (TryToTransitState updates the
	// tracker unconditionally, so the next frame's barrier emits a from-
	// state that no longer matches what D3D12 has on the resource — the
	// adapter's borrowed-output shells force this pass to skip frame 1
	// because the shells only activate at adapter Initialize, which
	// happens lazily inside PTNRDDenoisePass.PrepareCommandList earlier
	// in the same frame).
	if (m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto  l_fmService    = g_Engine->Get<FrameManagementService>();
	auto  l_resolution   = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto  l_perFrameCB   = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto* l_pt           = &GPUPathTracerPass::Get();
	auto* l_nrd          = &PTNRDDenoisePass::Get();

	auto* l_RT0          = l_pt->GetPTGBufferPosition();
	auto* l_RT2          = l_pt->GetPTGBufferAlbedoRoughness();
	auto* l_outDiff      = l_nrd->GetOutDiffRadianceHitDist();
	auto* l_outSpec      = l_nrd->GetOutSpecRadianceHitDist();
	auto* l_accumBuffer  = static_cast<TextureComponent*>(l_pt->GetResult());

	if (!l_RT0 || !l_RT2 || !l_outDiff || !l_outSpec || !l_accumBuffer)
		return false;

	// Graphics CL: transition the composition output UAV from ReadOnly
	// (cross-frame state) to ReadWrite. The 5 SRV inputs are already in the
	// SHADER_RESOURCE state — RT0 / RT2 set by the path tracer's end-of-
	// dispatch transitions, OUT_DIFF / OUT_SPEC set by the adapter's per-
	// dispatch tail transition, AccumBuffer set by the path tracer's
	// post-dispatch transition. TryToTransitState is a no-op when src/dst
	// match.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	// Compute CL: bind + dispatch.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_perFrameCB,    0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_RT0,           1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_RT2,           2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_outDiff,       3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_outSpec,       4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_accumBuffer,   5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result,        6);

	const uint32_t l_groupX = (l_resolution.x + 7u) / 8u;
	const uint32_t l_groupY = (l_resolution.y + 7u) / 8u;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groupX, l_groupY, 1u);

	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

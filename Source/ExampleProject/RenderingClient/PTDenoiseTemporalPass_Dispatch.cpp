#include "PTDenoiseTemporalPass.h"

#include "GPUPathTracerPass.h"
#include "PTDenoiseConstants.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool PTDenoiseTemporalPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if constexpr (!Inno::PTDenoise::ENABLED)
		return true;

	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_perFrameCB = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_owner      = &GPUPathTracerPass::Get();

	auto l_currRT0 = l_owner->GetCurrentPTGBufferPosition();
	auto l_currRT1 = l_owner->GetCurrentPTGBufferNormalMetalness();
	auto l_currRT3 = l_owner->GetCurrentPTGBufferMotionHitDist();
	auto l_prevRT0 = l_owner->GetPreviousPTGBufferPosition();
	auto l_prevRT1 = l_owner->GetPreviousPTGBufferNormalMetalness();

	auto l_currHistRadDiff  = GetCurrentHistoryRadianceDiffuse();
	auto l_currHistMomDiff  = GetCurrentHistoryMomentsDiffuse();
	auto l_currHistRadSpec  = GetCurrentHistoryRadianceSpecular();
	auto l_currHistMomSpec  = GetCurrentHistoryMomentsSpecular();
	auto l_prevHistRadDiff  = GetPreviousHistoryRadianceDiffuse();
	auto l_prevHistMomDiff  = GetPreviousHistoryMomentsDiffuse();
	auto l_prevHistRadSpec  = GetPreviousHistoryRadianceSpecular();
	auto l_prevHistMomSpec  = GetPreviousHistoryMomentsSpecular();

	// Graphics CL: flip read targets to ReadOnly, write targets to
	// WriteOnly. The current-frame radiance UAVs were transitioned to
	// ReadOnly by GPUPathTracerPass at the end of its dispatch, so they
	// already match the read-target state below — TryToTransitState is a
	// no-op when the transition source/dest already matches.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(l_currRT0,            m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_currRT1,            m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_currRT3,            m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_RadianceDiffuse,    m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_RadianceSpecular,   m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_prevRT0,            m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_prevRT1,            m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_prevHistRadDiff,    m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_prevHistMomDiff,    m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_prevHistRadSpec,    m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_prevHistMomSpec,    m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_currHistRadDiff,    m_CommandListComp_Graphics, Accessibility::ReadOnly,  Accessibility::WriteOnly);
	l_fmService->TryToTransitState(l_currHistMomDiff,    m_CommandListComp_Graphics, Accessibility::ReadOnly,  Accessibility::WriteOnly);
	l_fmService->TryToTransitState(l_currHistRadSpec,    m_CommandListComp_Graphics, Accessibility::ReadOnly,  Accessibility::WriteOnly);
	l_fmService->TryToTransitState(l_currHistMomSpec,    m_CommandListComp_Graphics, Accessibility::ReadOnly,  Accessibility::WriteOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_perFrameCB,        0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currRT0,           1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currRT1,           2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currRT3,           3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_RadianceDiffuse,   4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_RadianceSpecular,  5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_prevRT0,           6);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_prevRT1,           7);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_prevHistRadDiff,   8);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_prevHistMomDiff,   9);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_prevHistRadSpec,  10);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_prevHistMomSpec,  11);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currHistRadDiff,  12);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currHistMomDiff,  13);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currHistRadSpec,  14);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_currHistMomSpec,  15);

	// One thread per pixel, 8×8 group. Ceiling-divide so a viewport
	// not a multiple of 8 still covers the right + bottom strip.
	const uint32_t l_groupX = (l_resolution.x + 7u) / 8u;
	const uint32_t l_groupY = (l_resolution.y + 7u) / 8u;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groupX, l_groupY, 1u);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

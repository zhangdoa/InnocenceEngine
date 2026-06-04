#include "PTNRDFormatConvertPass.h"

#include "PTPass.h"
#include "NRDConstants.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool PTNRDFormatConvertPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto  l_fmService   = g_Engine->Get<FrameManagementService>();
	auto  l_resolution  = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto  l_perFrameCB  = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto* l_owner       = &PTPass::Get();

	auto* l_RT0 = l_owner->GetPTGBufferPosition();
	auto* l_RT1 = l_owner->GetPTGBufferNormalMetalness();
	auto* l_RT2 = l_owner->GetPTGBufferAlbedoRoughness();
	auto* l_RT3 = l_owner->GetPTGBufferMotionHitDist();
	auto* l_radDiff = l_owner->GetPTRadianceDiffuse();
	auto* l_radSpec = l_owner->GetPTRadianceSpecular();

	if (!l_RT0 || !l_RT1 || !l_RT2 || !l_RT3 || !l_radDiff || !l_radSpec)
		return false;

	// Graphics CL: transition our five output UAVs from ReadOnly (the
	// post-Initialize / cross-frame state) to ReadWrite for the kernel
	// write. The six SRV inputs were already transitioned to ReadOnly by
	// PTPass at the end of its compute dispatch (same-queue),
	// so we do not touch their state — TryToTransitState is a no-op when
	// the source/dest match. The graphics-queue transition itself is
	// required because tracked state may include PIXEL_SHADER_RESOURCE
	// (set by swap chain presentation paths), which is invalid on a
	// compute command list — same shape the path tracer's pre-pass uses.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_NRD_ViewZ,                m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->TryToTransitState(m_NRD_NormalRoughness,      m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->TryToTransitState(m_NRD_MotionVector,         m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->TryToTransitState(m_NRD_DiffRadianceHitDist,  m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->TryToTransitState(m_NRD_SpecRadianceHitDist,  m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	// Compute CL: bind + dispatch. Binding-slot layout mirrors the HLSL
	// register block exactly (PTNRDFormatConvert.comp): b0, t0..t5, u0..u4.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_perFrameCB,                0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_RT0,                       1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_RT1,                       2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_RT2,                       3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_RT3,                       4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_radDiff,                   5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_radSpec,                   6);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_NRD_ViewZ,                 7);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_NRD_NormalRoughness,       8);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_NRD_MotionVector,          9);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_NRD_DiffRadianceHitDist,  10);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_NRD_SpecRadianceHitDist,  11);

	// One thread per pixel, 8×8 group; ceiling-divide so a viewport not a
	// multiple of 8 still covers the right + bottom strip. Mirrors the
	// kernel's [numthreads(8, 8, 1)] declaration.
	const uint32_t l_groupX = (l_resolution.x + 7u) / 8u;
	const uint32_t l_groupY = (l_resolution.y + 7u) / 8u;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groupX, l_groupY, 1u);

	// Transition the five output UAVs back to ReadOnly so the next-frame
	// graphics CL pre-pass (the ReadOnly -> ReadWrite flip above) has a
	// well-defined source state. CL-3's NRD denoise pass will adopt these
	// five textures as SRV inputs and the ReadOnly state is already what
	// it expects.
	l_fmService->TryToTransitState(m_NRD_ViewZ,                m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_NRD_NormalRoughness,      m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_NRD_MotionVector,         m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_NRD_DiffRadianceHitDist,  m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_NRD_SpecRadianceHitDist,  m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

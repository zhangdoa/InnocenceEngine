#include "SSRCReprojectionPass.h"
#include "SSRCConstants.h"

#include "../../Engine/Services/PerFrameDataService.h"

#include "OpaquePass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool SSRCReprojectionPass::Initialize()
{

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SSRCReprojectionPass::Terminate()
{

	g_Engine->Get<TextureResourceService>()->Delete(m_ProbeMask);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Odd);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Odd);
	g_Engine->Get<TextureResourceService>()->Delete(m_SSRC_Even);
	g_Engine->Get<TextureResourceService>()->Delete(m_SSRC_Odd);

	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SSRCReprojectionPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SSRCReprojectionPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (m_SSRC_Even->m_ObjectStatus != ObjectStatus::Activated
		|| m_SSRC_Odd->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "SSRC Even/Odd not Activated, skipping.");
		return false;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_readTexture = GetPreviousFrameResult();
	auto l_writeTexture = GetCurrentFrameResult();
	auto l_probePosition = GetPreviousProbePosition();
	auto l_probeNormal = GetPreviousProbeNormal();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(l_readTexture, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_probePosition, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_probeNormal, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_writeTexture, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->TryToTransitState(m_ProbeMask, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
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
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_ProbeMask, 8);

	auto dispatch_x = SSRC::TileCount(l_writeTexture->m_TextureDesc.Width);
	auto dispatch_y = SSRC::TileCount(l_writeTexture->m_TextureDesc.Height);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, dispatch_x, dispatch_y, 1);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;


	return true;
}

RenderPassComponent* SSRCReprojectionPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* SSRCReprojectionPass::GetCurrentFrameResult()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_SSRC_Odd : m_SSRC_Even;
}

TextureComponent* SSRCReprojectionPass::GetPreviousFrameResult()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_SSRC_Even : m_SSRC_Odd;
}

TextureComponent* SSRCReprojectionPass::GetCurrentProbePosition()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbePosition_Odd : m_ProbePosition_Even;
}

TextureComponent* Inno::SSRCReprojectionPass::GetPreviousProbePosition()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbePosition_Even : m_ProbePosition_Odd;
}

TextureComponent* SSRCReprojectionPass::GetCurrentProbeNormal()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbeNormal_Odd : m_ProbeNormal_Even;
}

TextureComponent* Inno::SSRCReprojectionPass::GetPreviousProbeNormal()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_frameCount = l_fmService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_ProbeNormal_Even : m_ProbeNormal_Odd;
}

TextureComponent* SSRCReprojectionPass::GetProbeMask()
{
	return m_ProbeMask;
}

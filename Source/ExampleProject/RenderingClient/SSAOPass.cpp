#include "SSAOPass.h"
#include "ScreenTileConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "OpaquePass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

using namespace Inno;

namespace
{
	// Graph mode records the transition prepass + dispatch and creates the
	// screen-sized Result. Kernel/noise/samplers carry init data, so they stay
	// imperatively created (SetupOwnedResources) and are imported by name.
	constexpr bool g_UseRenderGraph = true;
}

bool SSAOPass::SetupFromRenderGraph()
{
	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("SSAONoisePass");
	if (!l_node)
	{
		Log(Error, "SSAOPass: render graph has no SSAONoisePass node.");
		return false;
	}

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_CommandListComp_Graphics = l_node->m_CommandList_Graphics;
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	if (!SetupOwnedResources())
		return false;

	// The deferred screen-sized Result is created when InitializeComponents runs
	// the node's RT init-func; resolved lazily in PrepareCommandList.
	m_Result = nullptr;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool SSAOPass::Setup(IServiceConfig* systemConfig)
{
	if (g_UseRenderGraph)
		return SetupFromRenderGraph();

	return SetupImperative();
}

bool SSAOPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp_RandomRot);
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_KernelGPUBuffer);
	g_Engine->Get<TextureResourceService>()->Initialize(m_NoiseTexture, &m_Noise[0]);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SSAOPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<TextureResourceService>()->Delete(m_Result);
	g_Engine->Get<TextureResourceService>()->Delete(m_NoiseTexture);
	g_Engine->Get<GPUBufferResourceService>()->Delete(m_KernelGPUBuffer);
	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp_RandomRot);
	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);
	
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SSAOPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SSAOPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (g_UseRenderGraph)
	{
		if (!m_Result)
			m_Result = static_cast<TextureComponent*>(g_Engine->Get<RenderGraphService>()->GetResource("SSAO_Result"));
		if (!m_Result || m_Result->m_ObjectStatus != ObjectStatus::Activated)
			return false;
		if (m_NoiseTexture->m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("SSAONoisePass");
		if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
			return false;

		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_NoiseTexture->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_currentFrame = l_fmService->GetCurrentFrame();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_NoiseTexture, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_KernelGPUBuffer, 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_NoiseTexture, 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_SamplerComp, 5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_SamplerComp_RandomRot, 6);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 7);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, uint32_t(l_viewportSize.x / static_cast<float>(ScreenTile::SCREEN_TILE_SIZE)), uint32_t(l_viewportSize.y / static_cast<float>(ScreenTile::SCREEN_TILE_SIZE)), 1);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SSAOPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SSAOPass::GetResult()
{
	return m_Result;
}

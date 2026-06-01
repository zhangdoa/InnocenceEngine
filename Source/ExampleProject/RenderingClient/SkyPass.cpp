#include "SkyPass.h"
#include "ScreenTileConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

using namespace Inno;

namespace
{
	// When true, SkyPass is driven by the render graph: its screen-sized Result is
	// created by the writer node's RT init-func (resize flows through the engine's
	// PostResize loop) and its dispatch comes from the screen-tile kernel. The
	// imperative path below is the fallback.
	constexpr bool g_UseRenderGraph = true;
}

bool SkyPass::SetupFromRenderGraph()
{
	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("SkyPass");
	if (!l_node)
	{
		Log(Error, "SkyPass: render graph has no SkyPass node.");
		return false;
	}

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	// m_Result (the deferred screen-sized RT) is created when Initialize runs the
	// node's RT-init-func; resolved after that in Initialize.
	m_Result = nullptr;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool SkyPass::Setup(IServiceConfig* systemConfig)
{
	if (g_UseRenderGraph)
		return SetupFromRenderGraph();

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SkyPass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "skyPass.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SkyPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&SkyPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(2);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// t0 - Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("SkyPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SkyPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	// RenderPassResourceService::Initialize is deferred — the node's RT-init-func
	// (which creates the screen-sized Result) runs later in InitializeComponents,
	// so m_Result is resolved lazily in PrepareCommandList, not here.
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SkyPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<TextureResourceService>()->Delete(m_Result);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SkyPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SkyPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (g_UseRenderGraph)
	{
		// The deferred RT was created by InitializeComponents (after Initialize);
		// resolve it for GetResult() consumers. ScreenTile kernel owns the binding
		// table + full-screen dispatch size.
		if (!m_Result)
			m_Result = static_cast<TextureComponent*>(g_Engine->Get<RenderGraphService>()->GetResource("Sky Pass Result"));
		if (!m_Result || m_Result->m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("SkyPass");
		if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
			return false;

		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "Result not Activated, skipping.");
		return false;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
	l_fmService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Compute);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 1);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, uint32_t(l_viewportSize.x / static_cast<float>(ScreenTile::SCREEN_TILE_SIZE)), uint32_t(l_viewportSize.y / static_cast<float>(ScreenTile::SCREEN_TILE_SIZE)), 1);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SkyPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SkyPass::GetResult()
{
	return m_Result;
}

bool SkyPass::RenderTargetsCreationFunc()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (m_Result)
		g_Engine->Get<TextureResourceService>()->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_Result = g_Engine->Get<TextureResourceService>()->Add("Sky Pass Result");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ComputeOnly;

	g_Engine->Get<TextureResourceService>()->Initialize(m_Result);

	return true;
}
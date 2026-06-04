#include "PreTAAPass.h"
#include "ScreenTileConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"


#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

using namespace Inno;


bool PreTAAPass::SetupFromRenderGraph()
{
	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("PreTAAPass");
	if (!l_node)
	{
		Log(Error, "PreTAAPass: render graph has no PreTAAPass node.");
		return false;
	}

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_CommandListComp_Graphics = l_node->m_CommandList_Graphics;
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	// The deferred screen-sized Result is created when InitializeComponents runs
	// the node's RT init-func; resolved lazily in PrepareCommandList.
	m_Result = nullptr;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PreTAAPass::Setup(IServiceConfig* systemConfig)
{
	return SetupFromRenderGraph();
}

bool PreTAAPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool PreTAAPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<TextureResourceService>()->Delete(m_Result);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);	
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus PreTAAPass::GetStatus()
{
	return m_ObjectStatus;
}

bool PreTAAPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (!m_Result)
		m_Result = static_cast<TextureComponent*>(g_Engine->Get<RenderGraphService>()->GetResource("Pre-TAA Pass Result"));
	if (!m_Result || m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("PreTAAPass");
	if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
		return false;

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* PreTAAPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* PreTAAPass::GetResult()
{
	return m_Result;
}

bool PreTAAPass::RenderTargetsCreationFunc()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (m_Result)
		g_Engine->Get<TextureResourceService>()->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_Result = g_Engine->Get<TextureResourceService>()->Add("Pre-TAA Pass Result");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ComputeOnly;

	g_Engine->Get<TextureResourceService>()->Initialize(m_Result);

	return true;
}
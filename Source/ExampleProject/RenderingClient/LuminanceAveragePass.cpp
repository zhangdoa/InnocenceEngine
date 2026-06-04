#include "LuminanceAveragePass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

using namespace Inno;

bool LuminanceAveragePass::SetupFromRenderGraph()
{
	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("LuminanceAveragePass");
	if (!l_node)
	{
		Log(Error, "LuminanceAveragePass: render graph has no LuminanceAveragePass node.");
		return false;
	}

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_luminanceAverage = static_cast<GPUBufferComponent*>(l_node->m_PrimaryOutput);
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LuminanceAveragePass::Setup(IServiceConfig* systemConfig)
{
	return SetupFromRenderGraph();
}

bool LuminanceAveragePass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	// Graph-driven path adopts components the RenderGraphService Added (resource +
	// render pass + command list); the graph does not Initialize them, so the
	// adopted pointers go through the same Initialize calls as the imperative path.
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_luminanceAverage);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LuminanceAveragePass::Update()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool LuminanceAveragePass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<GPUBufferResourceService>()->Delete(m_luminanceAverage);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LuminanceAveragePass::GetStatus()
{
	return m_ObjectStatus;
}

bool LuminanceAveragePass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (m_luminanceAverage->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("LuminanceAveragePass");
	if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
		return false;

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* LuminanceAveragePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* LuminanceAveragePass::GetResult()
{
	return m_luminanceAverage;
}
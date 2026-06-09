#include "ComputeCullingPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

#include <string>

using namespace Inno;

namespace
{
	const char* const k_IndirectBufferSuffix = "/IndirectDrawCommandBuffer";
}

bool ComputeCullingPass::SetupFromRenderGraph()
{
	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode(GetPassName());
	if (!l_node)
		return false;

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool ComputeCullingPass::Setup(IServiceConfig* systemConfig)
{
	return SetupFromRenderGraph();
}

bool ComputeCullingPass::Initialize()
{
	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool ComputeCullingPass::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus ComputeCullingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool ComputeCullingPass::PrepareCommandList(IRenderingContext* /*renderingContext*/)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode(GetPassName());
	if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
		return false;

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* ComputeCullingPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* ComputeCullingPass::GetResult()
{
	return g_Engine->Get<RenderGraphService>()->GetResource(std::string(GetPassName()) + k_IndirectBufferSuffix);
}

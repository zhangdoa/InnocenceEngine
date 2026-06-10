#include "ExampleRenderingClient_Internal.h"

#include "../../Engine/RenderGraph/RenderGraphService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	bool ExampleRenderingClientImpl::PrepareCommands()
	{
		// The render graph owns recording + submission (ExecuteCommands -> Render()).
		// Here we only resolve the present target by name from the graph.
		auto l_graph = g_Engine->Get<RenderGraphService>();
		m_Canvas = l_graph->GetResource("Final Blend Pass Result");
		auto l_node = l_graph->FindNode("FinalBlendPass");
		m_CanvasOwner = l_node ? l_node->m_RenderPass : nullptr;
		return true;
	}
}

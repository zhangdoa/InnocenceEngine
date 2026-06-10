#include "ExampleRenderingClient_Internal.h"

#include "../../Engine/RenderGraph/RenderGraphService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	bool ExampleRenderingClientImpl::ExecuteCommands(IRenderingConfig* renderingConfig)
	{
		// The render graph owns the whole frame: record every scheduled node, then
		// submit + fence them (queue/sync derived from node data). No per-pass code.
		g_Engine->Get<RenderGraphService>()->Render();

		HandleScreenCapture();
		HandleAutoCaptureTriggers();
		HandleAuditTrigger();

		return true;
	}
}

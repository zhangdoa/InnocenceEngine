#include "RenderGraphTransitions.h"
#include "RenderGraphService.h"

#include "../Engine.h"
#include "../Services/FrameManagementService.h"
#include "../Component/GPUResourceCast.h"
using namespace Inno;

bool Inno::RecordTransitionPrepass(RenderGraphPassContext& ctx, CommandListComponent* graphicsCL, FrameManagementService* fmService)
{
	if (!ctx.m_Node || !ctx.m_RenderPass || !graphicsCL || !fmService)
	{
		Log(Warning, "RecordTransitionPrepass rejected: incomplete prepass context.");
		return false;
	}

	auto l_graphService = g_Engine->Get<RenderGraphService>();

	for (const auto& l_transition : ctx.m_Node->m_Transitions)
	{
		if (!l_graphService->GetResource(l_transition.m_Resource))
		{
			Log(Error, "RecordTransitionPrepass [", ctx.m_Node->m_Name.c_str(),
				"]: transition resource [", l_transition.m_Resource.c_str(),
				"] unresolved; aborting pass to avoid dispatch with a missing barrier.");
			return false;
		}
	}

	fmService->CommandListBegin(ctx.m_RenderPass, graphicsCL, 0);
	for (const auto& l_transition : ctx.m_Node->m_Transitions)
	{
		auto* l_resource = l_transition.m_PingPongHistory
			? l_graphService->GetHistoryResource(l_transition.m_Resource)
			: l_graphService->GetResource(l_transition.m_Resource);
		// Branch on the resource's own type discriminator via the silent probe —
		// the binding-declared type is never trusted for the cast.
		if (auto* l_buffer = l_resource->TryAs<GPUBufferComponent>())
			fmService->TryToTransitState(l_buffer, graphicsCL, l_transition.m_From, l_transition.m_To);
		else if (auto* l_texture = l_resource->TryAs<TextureComponent>())
			fmService->TryToTransitState(l_texture, graphicsCL, l_transition.m_From, l_transition.m_To);
	}
	fmService->CommandListEnd(ctx.m_RenderPass, graphicsCL);
	return true;
}

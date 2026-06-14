#include "RenderGraphTransitions.h"
#include "RenderGraphService.h"

#include "../Engine.h"
#include "../Services/FrameManagementService.h"
#include "../Component/TextureComponent.h"
#include "../Component/GPUBufferComponent.h"
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
		// Reinterpret_cast across the GPUResourceComponent hierarchy is unsafe:
		// TextureComponent and GPUBufferComponent have different layouts, so
		// treating a buffer as a texture (or vice versa) reads garbage fields
		// and faults on the first pointer deref. Dispatch on the resource's
		// own m_GPUResourceType discriminator to pick the right overload.
		if (l_resource->m_GPUResourceType == GPUResourceType::Buffer)
			fmService->TryToTransitState(reinterpret_cast<GPUBufferComponent*>(l_resource),
				graphicsCL, l_transition.m_From, l_transition.m_To);
		else
			fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(l_resource),
				graphicsCL, l_transition.m_From, l_transition.m_To);
	}
	fmService->CommandListEnd(ctx.m_RenderPass, graphicsCL);
	return true;
}

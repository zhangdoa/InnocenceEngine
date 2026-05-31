#include "DefaultKernel.h"
#include "../Engine.h"
#include "../Services/FrameManagementService.h"

using namespace Inno;

bool DefaultKernel::Record(RenderGraphPassContext& ctx)
{
	if (!ctx.m_RenderPass || !ctx.m_CommandList || !ctx.m_Node)
	{
		Log(Warning, "DefaultKernel::Record rejected: incomplete pass context.");
		return false;
	}

	if (ctx.m_RenderPass->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "DefaultKernel::Record rejected for [", ctx.m_Node->m_Name.c_str(),
			"]: RenderPass not Activated.");
		return false;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(ctx.m_RenderPass, ctx.m_CommandList, 0);
	l_fmService->BindRenderPassComponent(ctx.m_RenderPass, ctx.m_CommandList);

	for (size_t i = 0; i < ctx.m_Node->m_Bindings.size(); i++)
	{
		auto l_resource = (i < ctx.m_BoundResources.size()) ? ctx.m_BoundResources[i] : nullptr;
		if (!l_resource)
		{
			Log(Warning, "DefaultKernel::Record [", ctx.m_Node->m_Name.c_str(),
				"]: binding ", i, " has no resolved resource.");
			continue;
		}
		l_fmService->BindGPUResource(ctx.m_RenderPass, ctx.m_CommandList,
			ctx.m_Node->m_Bindings[i].m_ShaderStage, l_resource, i);
	}

	uint32_t l_x = ctx.m_Node->m_Dispatch.m_X;
	uint32_t l_y = ctx.m_Node->m_Dispatch.m_Y;
	uint32_t l_z = ctx.m_Node->m_Dispatch.m_Z;
	ResolveDispatch(ctx, l_x, l_y, l_z);
	l_fmService->Dispatch(ctx.m_RenderPass, ctx.m_CommandList, l_x, l_y, l_z);

	l_fmService->CommandListEnd(ctx.m_RenderPass, ctx.m_CommandList);
	return true;
}

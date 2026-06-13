#include "RenderGraphPassRecorder.h"
#include "RenderGraphTransitions.h"
#include "../Engine.h"
#include "../Services/FrameManagementService.h"
#include "../Services/RenderingConfigurationService.h"
#include "../Services/DrawCallService.h"
#include "../Component/GPUBufferComponent.h"

#include <cmath>

using namespace Inno;

namespace
{
	// Dispatch thread-group count is a DispatchDesc Mode + tile size, not a kernel
	// subclass. Static keeps the literal X/Y/Z. ScreenTile floors viewport/tile
	// (screen-tile-aligned grid). TiledTwoLevel applies the light-culling
	// floor-then-ceil reduction. DrawModelGroups packs the live draw-model count
	// into groups of TileSize (replaces the old culling kernel's dynamic dispatch).
	void ApplyModeDispatch(const DispatchDesc& d, uint32_t& x, uint32_t& y, uint32_t& z)
	{
		if (d.m_Mode == DispatchMode::Static)
			return;

		if (d.m_Mode == DispatchMode::DrawModelGroups)
		{
			uint32_t l_count = static_cast<uint32_t>(g_Engine->Get<DrawCallService>()->GetGPUModelData().size());
			x = (l_count + d.m_TileSize - 1) / d.m_TileSize;
			y = 1;
			z = 1;
			return;
		}

		if (d.m_Mode == DispatchMode::DispatchRays)
		{
			auto l_res = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
			x = static_cast<uint32_t>(l_res.x);
			y = static_cast<uint32_t>(l_res.y);
			z = 1;
			return;
		}
		auto l_vp = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		if (d.m_Mode == DispatchMode::ScreenTile)
		{
			x = static_cast<uint32_t>(l_vp.x / static_cast<float>(d.m_TileSize));
			y = static_cast<uint32_t>(l_vp.y / static_cast<float>(d.m_TileSize));
		}
		else if (d.m_Mode == DispatchMode::TiledDispatch)
		{
			uint32_t l_effTile = d.m_TileSize * d.m_DispatchScale;
			x = (static_cast<uint32_t>(l_vp.x) + l_effTile - 1) / l_effTile;
			y = (static_cast<uint32_t>(l_vp.y) + l_effTile - 1) / l_effTile;
		}
		else
		{
			uint32_t l_threadsX = l_vp.x / d.m_TileSize;
			uint32_t l_threadsY = l_vp.y / d.m_TileSize;
			x = static_cast<uint32_t>(std::ceil(l_threadsX / static_cast<float>(d.m_TileSize)));
			y = static_cast<uint32_t>(std::ceil(l_threadsY / static_cast<float>(d.m_TileSize)));
		}
		z = 1;
	}

	// The node's first Writes resource, resolved to its live bound buffer handle.
	GPUBufferComponent* OutputBuffer(const RenderGraphPassContext& ctx)
	{
		if (ctx.m_Node->m_Writes.empty())
			return nullptr;

		const auto& l_name = ctx.m_Node->m_Writes[0];
		for (size_t i = 0; i < ctx.m_Node->m_Bindings.size() && i < ctx.m_BoundResources.size(); i++)
		{
			auto l_resource = ctx.m_BoundResources[i];
			if (ctx.m_Node->m_Bindings[i].m_Resource == l_name && l_resource
				&& l_resource->m_GPUResourceType == GPUResourceType::Buffer)
				return static_cast<GPUBufferComponent*>(l_resource);
		}
		return nullptr;
	}

	// Raster body: a graphics-queue node draws into its OutputMergerTarget via
	// ExecuteIndirect. Clears the targets, binds the shared resources (skipping
	// root-constant slots, whose value rides the indirect command signature), then
	// issues the indirect draw. The RT->COMMON exit and indirect-arg barriers are
	// emitted by the engine (PostCLState / ExecuteIndirect), not here.
	bool RecordRasterPass(const RenderGraphPassContext& ctx, FrameManagementService* l_fmService)
	{
		if (!ctx.m_IndirectArgs)
		{
			Log(Warning, "RecordPass [", ctx.m_Node->m_Name.c_str(), "]: raster node missing indirect-args buffer.");
			return false;
		}
		if (ctx.m_BoundResources.size() != ctx.m_Node->m_Bindings.size())
		{
			Log(Warning, "RecordPass [", ctx.m_Node->m_Name.c_str(), "]: bound-resource count ",
				ctx.m_BoundResources.size(), " != binding count ", ctx.m_Node->m_Bindings.size(), ".");
			return false;
		}

		l_fmService->CommandListBegin(ctx.m_RenderPass, ctx.m_CommandList, 0);
		l_fmService->BindRenderPassComponent(ctx.m_RenderPass, ctx.m_CommandList);
		l_fmService->ClearRenderTargets(ctx.m_RenderPass, ctx.m_CommandList);

		for (size_t i = 0; i < ctx.m_Node->m_Bindings.size(); i++)
		{
			if (ctx.m_Node->m_Bindings[i].m_IsRootConstant)
				continue;
			l_fmService->BindGPUResource(ctx.m_RenderPass, ctx.m_CommandList,
				ctx.m_Node->m_Bindings[i].m_ShaderStage, ctx.m_BoundResources[i], i);
		}

		l_fmService->ExecuteIndirect(ctx.m_RenderPass, ctx.m_CommandList,
			static_cast<GPUBufferComponent*>(ctx.m_IndirectArgs));
		l_fmService->CommandListEnd(ctx.m_RenderPass, ctx.m_CommandList);
		return true;
	}
}

bool Inno::RecordPass(RenderGraphPassContext& ctx)
{
	if (!ctx.m_RenderPass || !ctx.m_CommandList || !ctx.m_Node)
	{
		Log(Warning, "RecordPass rejected: incomplete pass context.");
		return false;
	}

	if (ctx.m_RenderPass->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RecordPass rejected for [", ctx.m_Node->m_Name.c_str(), "]: RenderPass not Activated.");
		return false;
	}

	if (ctx.m_Node->m_Raster.m_Enabled)
		return RecordRasterPass(ctx, g_Engine->Get<FrameManagementService>());

	uint32_t l_x = ctx.m_Node->m_Dispatch.m_X;
	uint32_t l_y = ctx.m_Node->m_Dispatch.m_Y;
	uint32_t l_z = ctx.m_Node->m_Dispatch.m_Z;
	ApplyModeDispatch(ctx.m_Node->m_Dispatch, l_x, l_y, l_z);

	// A count-driven dispatch of zero groups has no work — skip recording (parity
	// with the imperative culling pass's empty-model-set early-out).
	if (ctx.m_Node->m_Dispatch.m_Mode == DispatchMode::DrawModelGroups && l_x == 0)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (!ctx.m_Node->m_Transitions.empty())
	{
		if (!RecordTransitionPrepass(ctx, ctx.m_CommandList_Graphics, l_fmService))
			return false;
	}

	l_fmService->CommandListBegin(ctx.m_RenderPass, ctx.m_CommandList, 0);
	l_fmService->BindRenderPassComponent(ctx.m_RenderPass, ctx.m_CommandList);

	// Each binding is bound to the root parameter equal to its position in the
	// pass's m_ResourceBindingLayoutDescs[] — the layout-array slot, NOT the HLSL
	// register (m_DescriptorIndex). The graph builds the layout + m_BoundResources
	// by iterating m_Bindings in the same order, so binding i maps to slot i.
	if (ctx.m_BoundResources.size() != ctx.m_Node->m_Bindings.size())
	{
		Log(Warning, "RecordPass [", ctx.m_Node->m_Name.c_str(), "]: bound-resource count ",
			ctx.m_BoundResources.size(), " != binding count ", ctx.m_Node->m_Bindings.size(), ".");
		return false;
	}

	for (size_t i = 0; i < ctx.m_Node->m_Bindings.size(); i++)
	{
		auto l_resource = ctx.m_BoundResources[i];
		if (!l_resource)
		{
			Log(Warning, "RecordPass [", ctx.m_Node->m_Name.c_str(), "]: binding ", i, " has no resolved resource.");
			continue;
		}
		l_fmService->BindGPUResource(ctx.m_RenderPass, ctx.m_CommandList,
			ctx.m_Node->m_Bindings[i].m_ShaderStage, l_resource, i);
	}

	if (ctx.m_Node->m_Dispatch.m_Mode == DispatchMode::DispatchRays)
		l_fmService->DispatchRays(ctx.m_RenderPass, ctx.m_CommandList, l_x, l_y, l_z);
	else
		l_fmService->Dispatch(ctx.m_RenderPass, ctx.m_CommandList, l_x, l_y, l_z);
	l_fmService->CommandListEnd(ctx.m_RenderPass, ctx.m_CommandList);

	// Publish the output's post-write state so a downstream consumer (e.g. a
	// graphics pass doing ExecuteIndirect) emits the right barrier. The compute
	// queue can't emit it; fence sync covers visibility, we only update the tracker.
	if (ctx.m_Node->m_TrackWriteState)
	{
		if (auto l_output = OutputBuffer(ctx))
			l_output->SetCurrentState(l_fmService->GetCurrentFrame(), l_output->m_WriteState);
	}

	return true;
}

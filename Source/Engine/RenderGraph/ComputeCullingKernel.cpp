#include "ComputeCullingKernel.h"
#include "../Engine.h"
#include "../Services/DrawCallService.h"
#include "../Services/FrameManagementService.h"
#include "../Component/GPUBufferComponent.h"

using namespace Inno;

namespace
{
	// Must match THREAD_GROUP_SIZE in the culling .comp shader.
	constexpr uint32_t kThreadGroupSize = 64;

	uint32_t ModelCount()
	{
		return static_cast<uint32_t>(g_Engine->Get<DrawCallService>()->GetGPUModelData().size());
	}

	// The culling output (RWStructuredBuffer) is the node's primary Writes
	// resource; resolve the live bound handle by matching that binding name.
	GPUBufferComponent* OutputBuffer(RenderGraphPassContext& ctx)
	{
		if (ctx.m_Node->m_Writes.empty())
			return nullptr;

		const auto& l_outputName = ctx.m_Node->m_Writes[0];
		for (size_t i = 0; i < ctx.m_Node->m_Bindings.size() && i < ctx.m_BoundResources.size(); i++)
		{
			auto l_resource = ctx.m_BoundResources[i];
			if (ctx.m_Node->m_Bindings[i].m_Resource == l_outputName && l_resource
				&& l_resource->m_GPUResourceType == GPUResourceType::Buffer)
				return static_cast<GPUBufferComponent*>(l_resource);
		}
		return nullptr;
	}
}

bool ComputeCullingKernel::ResolveDispatch(RenderGraphPassContext&, uint32_t& x, uint32_t& y, uint32_t& z)
{
	uint32_t l_modelCount = ModelCount();
	uint32_t l_threadGroups = (l_modelCount + kThreadGroupSize - 1) / kThreadGroupSize;
	x = l_threadGroups > 0 ? l_threadGroups : 1;
	y = 1;
	z = 1;
	return true;
}

bool ComputeCullingKernel::Record(RenderGraphPassContext& ctx)
{
	// Empty model set: nothing to cull. Matches the imperative pass's early-out
	// (it returned before recording any commands).
	if (ModelCount() == 0)
		return false;

	if (!DefaultKernel::Record(ctx))
		return false;

	// CPU-side state tracking → UAV so the downstream graphics pass emits a
	// UAV→INDIRECT_ARGUMENT barrier before ExecuteIndirect. The compute queue
	// can't emit that transition; fence sync covers memory visibility, we only
	// update the tracker.
	if (auto l_output = OutputBuffer(ctx))
	{
		auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
		l_output->SetCurrentState(l_currentFrame, l_output->m_WriteState);
	}

	return true;
}

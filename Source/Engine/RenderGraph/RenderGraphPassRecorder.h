#pragma once
#include "../Common/Array.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/CommandListComponent.h"
#include "../Component/GPUResourceComponent.h"
#include "RenderGraphDesc.h"

namespace Inno
{
	// Per-pass resolved state: the graph resolves the data-declared bindings to
	// live resources + the desc's command lists, the recorder issues the work.
	struct RenderGraphPassContext
	{
		const PassNodeDesc* m_Node = nullptr;
		RenderPassComponent* m_RenderPass = nullptr;
		CommandListComponent* m_CommandList = nullptr;
		CommandListComponent* m_CommandList_Graphics = nullptr;
		// 1:1 with m_Node->m_Bindings — the live resource for each binding slot.
		Inno::Array<GPUResourceComponent*> m_BoundResources;
	};

	// Records a node's command list entirely from its data — no per-pass code.
	// Variation (dispatch shape, optional transition prepass, optional output
	// write-state publish) is all node data.
	bool RecordPass(RenderGraphPassContext& ctx);
}

#pragma once
#include "../Common/Array.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/CommandListComponent.h"
#include "../Component/GPUResourceComponent.h"
#include "RenderGraphDesc.h"

namespace Inno
{
	// Per-pass resolved state handed to a kernel at record time. The graph
	// resolves the data-declared bindings to live resources and the desc's
	// command list; the kernel issues the actual Dispatch/Draw.
	struct RenderGraphPassContext
	{
		const PassNodeDesc* m_Node = nullptr;
		RenderPassComponent* m_RenderPass = nullptr;
		CommandListComponent* m_CommandList = nullptr;
		// The pass's graphics command list — carries the state-transition prepass
		// (m_Node->m_Transitions) when present.
		CommandListComponent* m_CommandList_Graphics = nullptr;
		// 1:1 with m_Node->m_Bindings — the live resource for each binding slot.
		Inno::Array<GPUResourceComponent*> m_BoundResources;
	};

	// A kernel supplies the command-recording body that data cannot express.
	// The DefaultKernel covers fixed binding-table + static-dispatch passes; passes
	// with dynamic dispatch or deferred RTs override the hooks below.
	class IRenderGraphKernel
	{
	public:
		virtual ~IRenderGraphKernel() = default;

		virtual bool Record(RenderGraphPassContext& ctx) = 0;

		// Dynamic dispatch size. Default: use the static DispatchDesc.
		virtual bool ResolveDispatch(RenderGraphPassContext&, uint32_t&, uint32_t&, uint32_t&) { return false; }

		// Deferred render-target creation. Default: none.
		virtual bool CreateRenderTargets(RenderGraphPassContext&) { return false; }
	};
}

#pragma once
#include "IRenderGraphKernel.h"

namespace Inno
{
	// Clean-pass kernel (RFC §6 bin-a): binds every declared resource in
	// binding-table order, then issues the static Dispatch from the node's
	// DispatchDesc. Zero per-pass C++ — the data fully describes the body.
	class DefaultKernel : public IRenderGraphKernel
	{
	public:
		bool Record(RenderGraphPassContext& ctx) override;
	};
}

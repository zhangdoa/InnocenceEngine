#pragma once
#include "DefaultKernel.h"

namespace Inno
{
	// DefaultKernel binding + record, plus a dynamic dispatch size
	// (ceil(modelCount / threadGroupSize)) and the post-dispatch UAV state-tracker
	// side effect the imperative ComputeCullingPass carried. modelCount comes from
	// DrawCallService at record time; an empty model set is a no-op.
	class ComputeCullingKernel : public DefaultKernel
	{
	public:
		bool Record(RenderGraphPassContext& ctx) override;
		bool ResolveDispatch(RenderGraphPassContext& ctx, uint32_t& x, uint32_t& y, uint32_t& z) override;
	};
}

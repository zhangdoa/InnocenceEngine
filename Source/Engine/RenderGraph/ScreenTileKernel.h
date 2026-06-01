#pragma once
#include "DefaultKernel.h"

namespace Inno
{
	// Dispatch size derived from the current screen resolution
	// (floor(viewport / tileSize)) — the pattern full-screen compute passes share.
	// Tile size mirrors the HLSL [numthreads(8,8,1)] literal; see ScreenTileConstants.h.
	class ScreenTileKernel : public DefaultKernel
	{
	public:
		bool ResolveDispatch(RenderGraphPassContext& ctx, uint32_t& x, uint32_t& y, uint32_t& z) override;
	};
}

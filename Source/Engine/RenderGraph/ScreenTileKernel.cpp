#include "ScreenTileKernel.h"
#include "../Engine.h"
#include "../Services/RenderingConfigurationService.h"

using namespace Inno;

namespace
{
	// Canonical value is the HLSL [numthreads(8,8,1)] literal shared by every
	// full-screen compute shader (skyPass.comp et al.). Engine/RenderGraph cannot
	// include the RenderingClient ScreenTileConstants.h mirror, so the literal is
	// restated here against the same source of truth.
	constexpr uint32_t kScreenTileSize = 8;
}

bool ScreenTileKernel::ResolveDispatch(RenderGraphPassContext&, uint32_t& x, uint32_t& y, uint32_t& z)
{
	// Floor division — matches the imperative passes' uint32_t(viewport / tile)
	// exactly (the shaders' grid is screen-tile-aligned, so no remainder tile).
	auto l_viewport = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	x = static_cast<uint32_t>(l_viewport.x / static_cast<float>(kScreenTileSize));
	y = static_cast<uint32_t>(l_viewport.y / static_cast<float>(kScreenTileSize));
	z = 1;
	return true;
}

#pragma once
#include "../../Common/ComponentHeaders.h"
#include "../../Common/STL14.h"

namespace Inno
{
	namespace AssimpTextureProcessor
	{
		// Create and save TextureComponent to disk, returns the component instance name (empty on failure).
		// modelBaseDir: directory of the source model file, relative to the engine working directory
		// (used to resolve texture paths that are relative to the model, not to Bin/)
		std::string CreateTextureComponent(const char* fileName, const char* modelBaseDir, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex, const char* exportName);
	}
}

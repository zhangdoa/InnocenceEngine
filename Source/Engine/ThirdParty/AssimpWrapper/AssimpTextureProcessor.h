#pragma once
#include "../../Common/ComponentHeaders.h"
#include "../../Common/STL14.h"

namespace Inno
{
	namespace AssimpTextureProcessor
	{
		// Create and save TextureComponent to disk, returns the component instance name (empty on failure)
		std::string CreateTextureComponent(const char* fileName, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex, const char* exportName);
	}
}

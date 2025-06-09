#pragma once
#include "../../Common/ComponentHeaders.h"

namespace Inno
{
	namespace AssimpTextureProcessor
	{
		// Create and save TextureComponent directly - returns pointer for linking
		TextureComponent* CreateTextureComponent(const char* fileName, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex, const char* exportName);
	}
}

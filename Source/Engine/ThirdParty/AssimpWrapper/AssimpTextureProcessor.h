#pragma once
#include "../../Common/ComponentHeaders.h"
#include "../../Common/STL14.h"
#include "../../Common/BCCompression.h"

namespace Inno
{
	namespace AssimpTextureProcessor
	{
		// Create and save TextureComponent to disk, returns the component instance name (empty on failure).
		// modelBaseDir: directory of the source model file, relative to the engine working directory
		// (used to resolve texture paths that are relative to the model, not to Bin/)
		// bc4Source picks the RGBA channel packed into BC4 for slot 2/3/4; default R for
		// separate single-channel PNGs broadcast to RGBA by STB, B/G for glTF MR packing.
		std::string CreateTextureComponent(const char* fileName, const char* modelBaseDir, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex, const char* exportName, TextureChannelSource bc4Source = TextureChannelSource::R);
	}
}

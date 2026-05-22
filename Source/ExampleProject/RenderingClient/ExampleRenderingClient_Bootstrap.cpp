#include "ExampleRenderingClient_Internal.h"

#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Common/IOService.h"

#include "../../Engine/Engine.h"

#include <string>

using namespace Inno;

namespace Inno
{
	void ExampleRenderingClientImpl::BootstrapAmbientCGTextures()
	{
		// Idempotent bootstrap of AmbientCG PBR sets that materials in
		// ExampleProject scenes reference. Each ImportTexture writes a
		// TextureComponent JSON + binary if the JSON is missing; existing
		// JSONs short-circuit the load → recompress → save chain.
		struct PBRSetSlot { const char* slotName; uint32_t slotIndex; bool isSRGB; };
		static const PBRSetSlot s_AmbientCGSlots[] = {
			{ "NormalGL",  0u, false },
			{ "Color",     1u, true  },
			{ "Metalness", 2u, false },
			{ "Roughness", 3u, false },
		};
		static const char* s_AmbientCGSets[] = { "Concrete007", "Ground037", "Metal032", "Tiles074" };
		auto* l_io = g_Engine->Get<IOService>();
		auto l_dataDir = l_io->GetDataDirectory();
		for (const char* setName : s_AmbientCGSets)
		{
			std::string l_setDir = std::string("../OriginalAssets/Textures/") + setName + "/";
			std::string l_baseName = std::string(setName) + "_1K-PNG";
			for (const auto& slot : s_AmbientCGSlots)
			{
				std::string l_pngPath = l_setDir + l_baseName + "_" + slot.slotName + ".png";
				if (!l_io->IsFileExist(l_pngPath.c_str()))
					continue;

				std::string l_instanceName = std::string(setName) + "_" + slot.slotName + ".TextureComponent";

				auto l_destJson = l_dataDir + AssetService::GetAssetFilePath(l_instanceName.c_str());
				if (l_io->IsFileExist(l_destJson.c_str()))
					continue;

				std::string l_absPath = l_dataDir + l_pngPath;
				AssetService::ImportTexture(l_absPath.c_str(),
					TextureSampler::Sampler2D, TextureUsage::Sample,
					slot.isSRGB, slot.slotIndex, l_instanceName.c_str());
			}
		}
	}
}

#include "ExampleRenderingClient.h"

#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Services/ConfigurationService.h"
#include "../../Engine/Common/IOService.h"

#include "../../Engine/Engine.h"

#include <string>

using namespace Inno;

namespace Inno
{
	void ExampleRenderingClient::BootstrapAmbientCGTextures()
	{
		struct PBRSetSlot { const char* slotName; uint32_t slotIndex; bool isSRGB; };
		static const PBRSetSlot s_AmbientCGSlots[] = {
			{ "NormalGL",  0u, false },
			{ "Color",     1u, true  },
			{ "Metalness", 2u, false },
			{ "Roughness", 3u, false },
		};
		auto* l_io = g_Engine->Get<IOService>();
		auto l_dataDir = l_io->GetDataDirectory();
		for (const auto& l_set : g_Engine->Get<ConfigurationService>()->GetAmbientCGSets())
		{
			const std::string setName = l_set.setName;
			std::string l_setDir = l_set.assetPath + "/";
			std::string l_baseName = setName + "_1K-PNG";
			for (const auto& slot : s_AmbientCGSlots)
			{
				std::string l_pngPath = l_setDir + l_baseName + "_" + slot.slotName + ".png";
				if (!l_io->IsFileExist(l_pngPath.c_str()))
					continue;

				std::string l_instanceName = setName + "_" + slot.slotName + ".TextureComponent";

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

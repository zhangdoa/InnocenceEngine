#pragma once
#include "../Common/InnoType.h"
#include "../Component/VisibleComponent.h"

namespace InnoFileSystemNS
{
	namespace AssetLoader
	{
		ModelMap loadModel(const std::string & fileName, bool AsyncUploadGPUResource = true);
		TextureDataComponent* loadTexture(const std::string& fileName);
	};
}
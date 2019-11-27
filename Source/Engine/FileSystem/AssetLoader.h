#pragma once
#include "../Common/InnoType.h"
#include "../Component/VisibleComponent.h"

namespace InnoFileSystemNS
{
	namespace AssetLoader
	{
		ModelMap loadModel(const char* fileName, bool AsyncUploadGPUResource = true);
		TextureDataComponent* loadTexture(const char* fileName);
	};
}
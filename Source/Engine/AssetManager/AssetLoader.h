#pragma once
#include "../Common/InnoType.h"
#include "../Component/VisibleComponent.h"

namespace InnoFileSystemNS
{
	namespace AssetLoader
	{
		Model* loadModel(const char* fileName, bool AsyncUploadGPUResource = true);
		TextureDataComponent* loadTexture(const char* fileName);
	};
}
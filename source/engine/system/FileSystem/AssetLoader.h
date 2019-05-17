#pragma once
#include "../../common/InnoType.h"
#include "../../component/VisibleComponent.h"

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	INNO_PRIVATE_SCOPE AssetLoader
	{
		ModelMap loadModel(const std::string & fileName);
		TextureDataComponent* loadTexture(const std::string& fileName);
	};
}
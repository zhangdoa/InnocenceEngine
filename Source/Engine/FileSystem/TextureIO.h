#pragma once
#include "../Common/InnoType.h"
#include "../Component/VisibleComponent.h"

namespace InnoFileSystemNS
{
	namespace TextureIO
	{
		TextureDataComponent* loadTexture(const std::string & fileName);
		bool saveTexture(const std::string & fileName, TextureDataComponent* TDC);
	};
}
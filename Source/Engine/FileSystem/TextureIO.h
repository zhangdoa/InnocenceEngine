#pragma once
#include "../Common/InnoType.h"
#include "../Component/VisibleComponent.h"

namespace InnoFileSystemNS
{
	namespace TextureIO
	{
		TextureDataComponent* loadTexture(const char* fileName);
		bool saveTexture(const char* fileName, TextureDataComponent* TDC);
	};
}
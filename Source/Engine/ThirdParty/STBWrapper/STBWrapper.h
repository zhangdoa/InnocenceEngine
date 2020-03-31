#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/VisibleComponent.h"

namespace STBWrapper
{
	TextureDataComponent* loadTexture(const char* fileName);
	bool saveTexture(const char* fileName, TextureDataComponent* TDC);
};

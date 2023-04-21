#pragma once
#include "../../Common/Type.h"
#include "../../Component/VisibleComponent.h"

namespace Inno
{
	namespace STBWrapper
	{
		TextureComponent* loadTexture(const char* fileName);
		bool saveTexture(const char* fileName, TextureComponent* TextureComp);
	};
}

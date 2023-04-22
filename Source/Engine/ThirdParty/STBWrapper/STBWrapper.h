#pragma once
#include "../../Common/Type.h"
#include "../../Component/VisibleComponent.h"

namespace Inno
{
	namespace STBWrapper
	{
		TextureComponent* LoadTexture(const char* fileName);
		bool SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData);
	};
}

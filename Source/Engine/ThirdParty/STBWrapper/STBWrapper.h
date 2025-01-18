#pragma once
#include "../../Component/ModelComponent.h"

namespace Inno
{
	namespace STBWrapper
	{
		TextureComponent* LoadTexture(const char* fileName);
		bool SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData);
	};
}

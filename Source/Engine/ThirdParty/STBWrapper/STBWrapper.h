#pragma once
#include "../../Component/TextureComponent.h"

namespace Inno
{
	namespace STBWrapper
	{
		void* Load(const char* fileName, TextureComponent& component);
		void Free(void* data);
		bool Save(const char* fileName, const TextureDesc& textureDesc, void* textureData);
	};
}

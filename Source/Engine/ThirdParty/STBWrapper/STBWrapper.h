#pragma once
#include "../../Component/ModelComponent.h"

namespace Inno
{
	namespace STBWrapper
	{
		void* Load(const char* fileName, TextureComponent& component);
		bool Save(const char* fileName, const TextureDesc& textureDesc, void* textureData);
	};
}

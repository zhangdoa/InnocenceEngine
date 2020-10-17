#pragma once
#include "../Common/InnoType.h"
#include "TextureDataComponent.h"
#include "MTTextureDataComponent.h"

namespace Inno
{
	class MTRenderPassDataComponent
	{
	public:
		std::vector<MTTextureDataComponent*> m_MTTDCs;
	};
}

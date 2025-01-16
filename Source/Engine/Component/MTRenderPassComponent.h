#pragma once
#include "TextureComponent.h"
#include "MTTextureComponent.h"

namespace Inno
{
	class MTRenderPassComponent
	{
	public:
		std::vector<MTTextureComponent*> m_MTTextureComps;
	};
}

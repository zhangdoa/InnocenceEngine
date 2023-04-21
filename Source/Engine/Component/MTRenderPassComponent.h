#pragma once
#include "../Common/Type.h"
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

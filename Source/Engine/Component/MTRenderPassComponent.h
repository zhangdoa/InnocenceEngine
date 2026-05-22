#pragma once
#include "../Common/Array.h"
#include "TextureComponent.h"
#include "MTTextureComponent.h"

namespace Inno
{
	class MTRenderPassComponent
	{
	public:
		Inno::Array<MTTextureComponent*> m_MTTextureComps;
	};
}

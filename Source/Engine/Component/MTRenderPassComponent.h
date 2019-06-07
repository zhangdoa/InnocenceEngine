#pragma once
#include "../Common/InnoType.h"
#include "TextureDataComponent.h"
#include "MTTextureDataComponent.h"

class MTRenderPassComponent
{
public:
	MTRenderPassComponent() {};
	~MTRenderPassComponent() {};

	std::vector<MTTextureDataComponent*> m_MTTDCs;
};

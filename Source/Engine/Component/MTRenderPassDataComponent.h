#pragma once
#include "../Common/InnoType.h"
#include "TextureDataComponent.h"
#include "MTTextureDataComponent.h"

class MTRenderPassDataComponent
{
public:
	MTRenderPassDataComponent() {};
	~MTRenderPassDataComponent() {};

	std::vector<MTTextureDataComponent*> m_MTTDCs;
};

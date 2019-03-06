#pragma once
#include "../common/InnoType.h"
#include "TextureDataComponent.h"
#include "VKTextureDataComponent.h"

class VKRenderPassComponent
{
public:
	VKRenderPassComponent() {};
	~VKRenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<VKTextureDataComponent*> m_VKTDCs;
};
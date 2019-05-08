#pragma once
#include "../common/InnoType.h"
#include "../common/InnoContainer.h"

#include "TextureDataComponent.h"

class RenderPassComponent
{
public:
	RenderPassComponent() {};
	~RenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;
	FixedSizeString<128> m_name;
};
#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

class GLTextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_TAO = 0;
};


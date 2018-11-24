#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

class GLTextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	GLuint m_TAO = 0;
};


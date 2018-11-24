#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

class GLFrameBufferComponent
{
public:
	GLFrameBufferComponent() {};
	~GLFrameBufferComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_FBO = 0;
	GLuint m_RBO = 0;
};
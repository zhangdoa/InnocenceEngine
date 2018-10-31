#pragma once
#include "../common/InnoType.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLFrameBufferComponent
{
public:
	GLFrameBufferComponent() {};
	~GLFrameBufferComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_FBO = 0;
	GLuint m_RBO = 0;
};
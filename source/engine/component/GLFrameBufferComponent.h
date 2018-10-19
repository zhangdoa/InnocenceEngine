#pragma once
#include "BaseComponent.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLFrameBufferComponent : public BaseComponent
{
public:
	GLFrameBufferComponent() {};
	~GLFrameBufferComponent() {};

	GLuint m_FBO = 0;
	GLuint m_RBO = 0;
};
#pragma once
#include "BaseComponent.h"
#include "../common/GLHeaders.h"

class GLFrameBufferComponent : public BaseComponent
{
public:
	GLFrameBufferComponent() {};
	~GLFrameBufferComponent() {};

	GLuint m_FBO;
	GLuint m_RBO;
};
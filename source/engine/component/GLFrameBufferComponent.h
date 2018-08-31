#pragma once
#include "BaseComponent.h"
#include "../system/GLRenderer/GLHeaders.h"

class GLFrameBufferComponent : public BaseComponent
{
public:
	GLFrameBufferComponent() {};
	~GLFrameBufferComponent() {};

	GLuint m_FBO;
	GLuint m_RBO;
};
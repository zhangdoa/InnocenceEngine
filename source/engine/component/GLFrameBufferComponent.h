#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"

class GLFrameBufferComponent : public BaseComponent
{
public:
	GLFrameBufferComponent() {};
	~GLFrameBufferComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;

	GLuint m_FBO;
	GLuint m_RBO;
};
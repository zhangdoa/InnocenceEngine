#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"

class GLShaderProgramComponent : public BaseComponent
{
public:
	GLShaderProgramComponent() {};
	~GLShaderProgramComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;

	GLuint m_program = 0;
};
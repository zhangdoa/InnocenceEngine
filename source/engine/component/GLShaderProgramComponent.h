#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"

class GLShaderProgramComponent : public BaseComponent
{
public:
	GLShaderProgramComponent() {};
	~GLShaderProgramComponent() {};

	GLuint m_program = 0;
};
#pragma once
#include "BaseComponent.h"
#include "../system/GLRenderer/GLHeaders.h"

class GLShaderProgramComponent : public BaseComponent
{
public:
	GLShaderProgramComponent() {};
	~GLShaderProgramComponent() {};

	GLuint m_program = 0;
};
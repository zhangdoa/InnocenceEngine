#pragma once
#include "BaseComponent.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLShaderProgramComponent : public BaseComponent
{
public:
	GLShaderProgramComponent() {};
	~GLShaderProgramComponent() {};

	GLuint m_program = 0;
};
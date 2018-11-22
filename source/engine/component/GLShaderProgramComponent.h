#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

class GLShaderProgramComponent
{
public:
	GLShaderProgramComponent() {};
	~GLShaderProgramComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	GLuint m_program = 0;
	GLuint m_VS = 0;
	GLuint m_GS = 0;
	GLuint m_FS = 0;
};
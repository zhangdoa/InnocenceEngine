#pragma once
#include "../common/InnoType.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLShaderProgramComponent
{
public:
	GLShaderProgramComponent() {};
	~GLShaderProgramComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_program = 0;
};
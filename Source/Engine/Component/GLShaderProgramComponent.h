#pragma once
#include "ShaderProgramComponent.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"

class GLShaderProgramComponent : public ShaderProgramComponent
{
public:
	GLShaderProgramComponent() {};
	~GLShaderProgramComponent() {};

	GLuint m_ProgramID = 0;
	GLuint m_VSID = 0;
	GLuint m_TCSID = 0;
	GLuint m_TESID = 0;
	GLuint m_GSID = 0;
	GLuint m_FSID = 0;
	GLuint m_CSID = 0;
};
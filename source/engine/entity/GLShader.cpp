#include "GLShader.h"

void GLShader::initialize()
{
	int l_glShaderType = 0;

	switch (std::get<shaderType>(m_shaderData))
	{
	case shaderType::VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case shaderType::GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case shaderType::FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: g_pLogSystem->printLog("Error: Unknown shader type, cannot create shader!"); m_objectStatus = objectStatus::STANDBY; break;
	}

	m_shaderID = glCreateShader(l_glShaderType);

	if (m_shaderID == 0) {
		g_pLogSystem->printLog("Error: Shader creation failed: memory location invaild when adding shader!");
		m_objectStatus = objectStatus::STANDBY;
	}

	char const * l_sourcePointer = std::get<shaderCodeContent>(m_shaderData).c_str();
	glShaderSource(m_shaderID, 1, &l_sourcePointer, NULL);
}

void GLShader::shutdown()
{
	glDeleteShader(m_shaderID);
	m_objectStatus = objectStatus::SHUTDOWN;
}

const GLint & GLShader::getShaderID() const
{
	return m_shaderID;
}

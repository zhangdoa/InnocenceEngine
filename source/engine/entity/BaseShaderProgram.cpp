#include "BaseShaderProgram.h"

void BaseShaderProgram::setup()
{
	g_pLogSystem->printLog("BaseShaderProgram: Warning: use the setup() with parameter!");
}

void BaseShaderProgram::setup(const std::vector<BaseShader*>& baseShaders)
{
	m_baseShaders = baseShaders;
	m_objectStatus = objectStatus::ALIVE;
}

const objectStatus & BaseShaderProgram::getStatus() const
{
	return m_objectStatus;
}

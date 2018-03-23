#include "BaseShaderProgram.h"

void BaseShaderProgram::setup()
{
	g_pLogSystem->printLog("BaseShaderProgram: Warning: use the setup() with parameter!");
}

void BaseShaderProgram::setup(const std::vector<shaderData>& shaderDatas)
{
	m_shaderDatas = shaderDatas;
	m_objectStatus = objectStatus::ALIVE;
}

void BaseShaderProgram::update()
{
	g_pLogSystem->printLog("BaseShaderProgram: Warning: use the update() with parameter!");
}

const objectStatus & BaseShaderProgram::getStatus() const
{
	return m_objectStatus;
}

#include "BaseShader.h"

void BaseShader::setup()
{
	g_pLogSystem->printLog("BaseShader: Warning: use the setup() with parameter!");
}

void BaseShader::setup(shaderData shaderData)
{
	m_shaderData = shaderData;
	m_objectStatus = objectStatus::ALIVE;
}

void BaseShader::update()
{
	g_pLogSystem->printLog("BaseShader: Warning: shader class do not need to update()! Use ShaderProgram's update().");
}

const objectStatus & BaseShader::getStatus() const
{
	return m_objectStatus;
}

const shaderData & BaseShader::getShaderData() const
{
	return m_shaderData;
}
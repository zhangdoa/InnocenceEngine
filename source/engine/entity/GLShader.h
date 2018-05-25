#pragma once
#include "common/GLHeaders.h"
#include "BaseShader.h"

class GLShader : public BaseShader
{
public:
	GLShader() {};
	~GLShader() {};

	void initialize() override;
	void shutdown() override;

	const int getShaderID() const override;

private:
	GLint m_shaderID = 0;
};
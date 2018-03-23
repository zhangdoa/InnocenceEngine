#pragma once
#include "BaseShader.h"

class GLShader : public BaseShader
{
public:
	GLShader() {};
	~GLShader() {};

	void initialize() override;
	void shutdown() override;

	const GLint & getShaderID() const;

private:
	GLint m_shaderID = 0;
};
#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"

class EnvironmentRenderPassSingletonComponent : public BaseComponent
{
public:
	~EnvironmentRenderPassSingletonComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;
	
	static EnvironmentRenderPassSingletonComponent& getInstance()
	{
		static EnvironmentRenderPassSingletonComponent instance;
		return instance;
	}

	GLuint m_FBO;
	GLuint m_RBO;

	GLuint m_capturePassProgram;
	GLuint m_capturePassVertexShaderID;
	GLuint m_capturePassFragmentShaderID;
	GLuint m_capturePassTextureID;
	GLuint m_capturePass_uni_equirectangularMap;
	GLuint m_capturePass_uni_p;
	GLuint m_capturePass_uni_r;

	GLuint m_convolutionPassProgram;
	GLuint m_convolutionPassVertexShaderID;
	GLuint m_convolutionPassFragmentShaderID;
	GLuint m_convolutionPassTextureID;

	GLuint m_preFilterPassProgram;
	GLuint m_preFilterPassVertexShaderID;
	GLuint m_preFilterPassFragmentShaderID;
	GLuint m_preFilterPassTextureID;

	GLuint m_BRDFLUTPassProgram;
	GLuint m_BRDFLUTPassVertexShaderID;
	GLuint m_BRDFLUTPassFragmentShaderID;
	GLuint m_BRDFLUTTextureID;

private:
	EnvironmentRenderPassSingletonComponent() {};
};

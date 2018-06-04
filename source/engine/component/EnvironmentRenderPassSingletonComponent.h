#pragma once
#include "BaseComponent.h"
#include "component/GLFrameBufferComponent.h"
#include "component/GLShaderProgramComponent.h"
#include "component/GLTextureDataComponent.h"

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

	GLFrameBufferComponent m_GLFrameBufferComponent;

	GLShaderProgramComponent m_capturePassProgram;
	GLuint m_capturePassVertexShaderID;
	GLuint m_capturePassFragmentShaderID;
	GLTextureDataComponent m_capturePassTexture;
	GLuint m_capturePass_uni_equirectangularMap;
	GLuint m_capturePass_uni_p;
	GLuint m_capturePass_uni_r;

	GLShaderProgramComponent m_convolutionPassProgram;
	GLuint m_convolutionPassVertexShaderID;
	GLuint m_convolutionPassFragmentShaderID;
	GLTextureDataComponent m_convolutionPassTexture;
	GLuint m_convolutionPass_uni_p;
	GLuint m_convolutionPass_uni_r;
	
	GLShaderProgramComponent m_preFilterPassProgram;
	GLuint m_preFilterPassVertexShaderID;
	GLuint m_preFilterPassFragmentShaderID;
	GLTextureDataComponent m_preFilterPassTexture;
	GLuint m_preFilterPass_uni_roughness;
	GLuint m_preFilterPass_uni_p;
	GLuint m_preFilterPass_uni_r;

	GLShaderProgramComponent m_BRDFLUTPassProgram;
	GLuint m_BRDFLUTPassVertexShaderID;
	GLuint m_BRDFLUTPassFragmentShaderID;
	GLTextureDataComponent m_BRDFLUTTexture;

private:
	EnvironmentRenderPassSingletonComponent() {};
};

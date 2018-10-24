#pragma once
#include "BaseComponent.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "GLTextureDataComponent.h"

class EnvironmentRenderPassSingletonComponent : public BaseComponent
{
public:
	~EnvironmentRenderPassSingletonComponent() {};
	
	static EnvironmentRenderPassSingletonComponent& getInstance()
	{
		static EnvironmentRenderPassSingletonComponent instance;
		return instance;
	}

	GLFrameBufferComponent* m_FBC;

	GLShaderProgramComponent* m_capturePassProgram;
	GLuint m_capturePassVertexShaderID;
	GLuint m_capturePassFragmentShaderID;
	TextureDataComponent* m_capturePassTDC;
	GLTextureDataComponent* m_capturePassGLTDC;
	GLuint m_capturePass_uni_equirectangularMap;
	GLuint m_capturePass_uni_p;
	GLuint m_capturePass_uni_r;

	GLShaderProgramComponent* m_convolutionPassProgram;
	GLuint m_convolutionPassVertexShaderID;
	GLuint m_convolutionPassFragmentShaderID;
	TextureDataComponent* m_convolutionPassTDC;
	GLTextureDataComponent* m_convolutionPassGLTDC;
	GLuint m_convolutionPass_uni_capturedCubeMap;
	GLuint m_convolutionPass_uni_p;
	GLuint m_convolutionPass_uni_r;
	
	GLShaderProgramComponent* m_preFilterPassProgram;
	GLuint m_preFilterPassVertexShaderID;
	GLuint m_preFilterPassFragmentShaderID;
	TextureDataComponent* m_preFilterPassTDC;
	GLTextureDataComponent* m_preFilterPassGLTDC;
	GLuint m_preFilterPass_uni_capturedCubeMap;
	GLuint m_preFilterPass_uni_roughness;
	GLuint m_preFilterPass_uni_p;
	GLuint m_preFilterPass_uni_r;

	GLShaderProgramComponent* m_BRDFLUTPassProgram;
	GLuint m_BRDFLUTPassVertexShaderID;
	GLuint m_BRDFLUTPassFragmentShaderID;
	TextureDataComponent* m_BRDFLUTTDC;
	GLTextureDataComponent* m_BRDFLUTGLTDC;
private:
	EnvironmentRenderPassSingletonComponent() {};
};

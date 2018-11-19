#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

class EnvironmentRenderPassSingletonComponent
{
public:
	~EnvironmentRenderPassSingletonComponent() {};
	
	static EnvironmentRenderPassSingletonComponent& getInstance()
	{
		static EnvironmentRenderPassSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFrameBufferComponent* m_FBC;

	GLShaderProgramComponent* m_capturePassSPC;
	TextureDataComponent* m_capturePassTDC;
	GLTextureDataComponent* m_capturePassGLTDC;
	GLuint m_capturePass_uni_equirectangularMap;
	GLuint m_capturePass_uni_p;
	GLuint m_capturePass_uni_r;

	GLShaderProgramComponent* m_convolutionPassSPC;
	TextureDataComponent* m_convolutionPassTDC;
	GLTextureDataComponent* m_convolutionPassGLTDC;
	GLuint m_convolutionPass_uni_capturedCubeMap;
	GLuint m_convolutionPass_uni_p;
	GLuint m_convolutionPass_uni_r;
	
	unsigned int m_maxMipLevels = 5;
	GLShaderProgramComponent* m_preFilterPassSPC;
	TextureDataComponent* m_preFilterPassTDC;
	GLTextureDataComponent* m_preFilterPassGLTDC;
	GLuint m_preFilterPass_uni_capturedCubeMap;
	GLuint m_preFilterPass_uni_roughness;
	GLuint m_preFilterPass_uni_p;
	GLuint m_preFilterPass_uni_r;

	GLShaderProgramComponent* m_BRDFLUTPassSPC;
	TextureDataComponent* m_BRDFLUTTDC;
	GLTextureDataComponent* m_BRDFLUTGLTDC;
private:
	EnvironmentRenderPassSingletonComponent() {};
};

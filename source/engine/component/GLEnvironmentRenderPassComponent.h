#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

class GLEnvironmentRenderPassComponent
{
public:
	~GLEnvironmentRenderPassComponent() {};
	
	static GLEnvironmentRenderPassComponent& get()
	{
		static GLEnvironmentRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFrameBufferComponent* m_BRDFLUTPassFBC;

	GLShaderProgramComponent* m_BRDFSplitSumLUTPassSPC;
	TextureDataComponent* m_BRDFSplitSumLUTPassTDC;
	GLTextureDataComponent* m_BRDFSplitSumLUTPassGLTDC;

	GLShaderProgramComponent* m_BRDFMSAverageLUTPassSPC;
	TextureDataComponent* m_BRDFMSAverageLUTPassTDC;
	GLTextureDataComponent* m_BRDFMSAverageLUTPassGLTDC;
	GLuint m_BRDFMSAverageLUTPass_uni_brdfLUT;

	ShaderFilePaths m_capturePassShaderFilePaths = { "GL4.0//environmentCapturePassVertex.sf" , "", "GL4.0//environmentCapturePassFragment.sf" };
	GLShaderProgramComponent* m_capturePassSPC;
	GLuint m_capturePass_uni_albedoTexture;
	GLuint m_capturePass_uni_p;
	GLuint m_capturePass_uni_v;
	GLuint m_capturePass_uni_m;
	GLuint m_capturePass_uni_useAlbedoTexture;
	GLuint m_capturePass_uni_albedo;
	GLFrameBufferComponent* m_capturePassFBC;
	TextureDataComponent* m_capturePassTDC;
	GLTextureDataComponent* m_capturePassGLTDC;

	ShaderFilePaths m_convPassShaderFilePaths = { "GL4.0//environmentConvolutionPassVertex.sf" , "", "GL4.0//environmentConvolutionPassFragment.sf" };
	GLShaderProgramComponent* m_convPassSPC;
	GLuint m_convPass_uni_capturedCubeMap;
	GLuint m_convPass_uni_p;
	GLuint m_convPass_uni_r;
	TextureDataComponent* m_convPassTDC;
	GLTextureDataComponent* m_convPassGLTDC;

	ShaderFilePaths m_preFilterPassShaderFilePaths = { "GL4.0//environmentPreFilterPassVertex.sf" , "", "GL4.0//environmentPreFilterPassFragment.sf" };
	GLShaderProgramComponent* m_preFilterPassSPC;
	GLuint m_preFilterPass_uni_capturedCubeMap;
	GLuint m_preFilterPass_uni_p;
	GLuint m_preFilterPass_uni_r;
	GLuint m_preFilterPass_uni_roughness;
	TextureDataComponent* m_preFilterPassTDC;
	GLTextureDataComponent* m_preFilterPassGLTDC;

private:
	GLEnvironmentRenderPassComponent() {};
};

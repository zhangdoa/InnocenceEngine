#pragma once
#include "BaseComponent.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "GLTextureDataComponent.h"

class GLFinalRenderPassSingletonComponent : public BaseComponent
{
public:
	~GLFinalRenderPassSingletonComponent() {};
	
	static GLFinalRenderPassSingletonComponent& getInstance()
	{
		static GLFinalRenderPassSingletonComponent instance;
		return instance;
	}
	GLFrameBufferComponent m_skyPassGLFrameBufferComponent;
	GLShaderProgramComponent m_skyPassProgram;
	GLuint m_skyPassVertexShaderID;
	GLuint m_skyPassFragmentShaderID;
	GLTextureDataComponent m_skyPassTexture;
	GLuint m_skyPass_uni_skybox;
	GLuint m_skyPass_uni_p;
	GLuint m_skyPass_uni_r;

	GLFrameBufferComponent m_TAAPingPassGLFrameBufferComponent;
	GLFrameBufferComponent m_TAAPongPassGLFrameBufferComponent;
	GLShaderProgramComponent m_TAAPassProgram;
	GLuint m_TAAPassVertexShaderID;
	GLuint m_TAAPassFragmentShaderID;
	GLTextureDataComponent m_TAAPingPassTexture;
	GLTextureDataComponent m_TAAPongPassTexture;
	GLuint m_TAAPass_uni_lightPassRT0;
	GLuint m_TAAPass_uni_lastTAAPassRT0;
	GLuint m_TAAPass_uni_motionVectorTexture;
	GLuint m_TAAPass_uni_renderTargetSize;

	GLFrameBufferComponent m_bloomExtractPassGLFrameBufferComponent;
	GLShaderProgramComponent m_bloomExtractPassProgram;
	GLuint m_bloomExtractPassVertexShaderID;
	GLuint m_bloomExtractPassFragmentShaderID;
	GLTextureDataComponent m_bloomExtractPassTexture;
	GLuint m_bloomExtractPass_uni_TAAPassRT0;

	GLFrameBufferComponent m_bloomBlurPingPassGLFrameBufferComponent;
	GLFrameBufferComponent m_bloomBlurPongPassGLFrameBufferComponent;
	GLShaderProgramComponent m_bloomBlurPassProgram;
	GLuint m_bloomBlurPassVertexShaderID;
	GLuint m_bloomBlurPassFragmentShaderID;
	GLTextureDataComponent m_bloomBlurPingPassTexture;
	GLTextureDataComponent m_bloomBlurPongPassTexture;
	GLuint m_bloomBlurPass_uni_bloomExtractPassRT0;
	GLuint m_bloomBlurPass_uni_horizontal;

	GLFrameBufferComponent m_motionBlurPassGLFrameBufferComponent;
	GLShaderProgramComponent m_motionBlurPassProgram;
	GLuint m_motionBlurPassVertexShaderID;
	GLuint m_motionBlurPassFragmentShaderID;
	GLTextureDataComponent m_motionBlurPassTexture;
	GLuint m_motionBlurPass_uni_motionVectorTexture;
	GLuint m_motionBlurPass_uni_TAAPassRT0;

	GLFrameBufferComponent m_billboardPassGLFrameBufferComponent;
	GLShaderProgramComponent m_billboardPassProgram;
	GLuint m_billboardPassVertexShaderID;
	GLuint m_billboardPassFragmentShaderID;
	GLTextureDataComponent m_billboardPassTexture;
	GLuint m_billboardPass_uni_texture;
	GLuint m_billboardPass_uni_p;
	GLuint m_billboardPass_uni_r;
	GLuint m_billboardPass_uni_t;
	GLuint m_billboardPass_uni_pos;
	GLuint m_billboardPass_uni_albedo;
	GLuint m_billboardPass_uni_size;

	GLFrameBufferComponent m_debuggerPassGLFrameBufferComponent;
	GLShaderProgramComponent m_debuggerPassProgram;
	GLuint m_debuggerPassVertexShaderID;
	GLuint m_debuggerPassFragmentShaderID;
	GLTextureDataComponent m_debuggerPassTexture;
	GLuint m_debuggerPass_uni_normalTexture;
	GLuint m_debuggerPass_uni_p;
	GLuint m_debuggerPass_uni_r;	
	GLuint m_debuggerPass_uni_t;	
	GLuint m_debuggerPass_uni_m;

	GLFrameBufferComponent m_finalBlendPassGLFrameBufferComponent;
	GLShaderProgramComponent m_finalBlendPassProgram;
	GLuint m_finalBlendPassVertexShaderID;
	GLuint m_finalBlendPassFragmentShaderID;
	GLTextureDataComponent m_finalBlendPassTexture;
	GLuint m_uni_motionBlurPassRT0;
	GLuint m_uni_skyPassRT0;
	GLuint m_uni_bloomPassRT0;
	GLuint m_uni_billboardPassRT0;
	GLuint m_uni_debuggerPassRT0;

private:
	GLFinalRenderPassSingletonComponent() {};
};

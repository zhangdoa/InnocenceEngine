#pragma once
#include "../common/InnoType.h"
#include "GLShaderProgramComponent.h"
#include "GLRenderPassComponent.h"

class GLFinalRenderPassSingletonComponent
{
public:
	~GLFinalRenderPassSingletonComponent() {};
	
	static GLFinalRenderPassSingletonComponent& getInstance()
	{
		static GLFinalRenderPassSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLRenderPassComponent* m_skyPassGLRPC;
	GLShaderProgramComponent* m_skyPassSPC;
	GLuint m_skyPass_uni_p;
	GLuint m_skyPass_uni_r;
	GLuint m_skyPass_uni_viewportSize;
	GLuint m_skyPass_uni_eyePos;	
	GLuint m_skyPass_uni_lightDir;

	GLRenderPassComponent* m_TAAPingPassGLRPC;
	GLRenderPassComponent* m_TAAPongPassGLRPC;
	GLShaderProgramComponent* m_TAAPassSPC;
	GLuint m_TAAPass_uni_lightPassRT0;
	GLuint m_TAAPass_uni_lastTAAPassRT0;
	GLuint m_TAAPass_uni_motionVectorTexture;
	GLuint m_TAAPass_uni_renderTargetSize;

	GLRenderPassComponent* m_bloomExtractPassGLRPC;
	GLShaderProgramComponent* m_bloomExtractPassSPC;
	GLuint m_bloomExtractPass_uni_TAAPassRT0;

	GLRenderPassComponent* m_bloomBlurPingPassGLRPC;
	GLRenderPassComponent* m_bloomBlurPongPassGLRPC;
	GLShaderProgramComponent* m_bloomBlurPassSPC;
	GLuint m_bloomBlurPass_uni_bloomExtractPassRT0;
	GLuint m_bloomBlurPass_uni_horizontal;

	GLRenderPassComponent* m_motionBlurPassGLRPC;
	GLShaderProgramComponent* m_motionBlurPassSPC;
	GLuint m_motionBlurPass_uni_motionVectorTexture;
	GLuint m_motionBlurPass_uni_TAAPassRT0;

	GLRenderPassComponent* m_billboardPassGLRPC;
	GLShaderProgramComponent* m_billboardPassSPC;
	GLuint m_billboardPass_uni_texture;
	GLuint m_billboardPass_uni_p;
	GLuint m_billboardPass_uni_r;
	GLuint m_billboardPass_uni_t;
	GLuint m_billboardPass_uni_pos;
	GLuint m_billboardPass_uni_albedo;
	GLuint m_billboardPass_uni_size;

	GLRenderPassComponent* m_debuggerPassGLRPC;
	GLShaderProgramComponent* m_debuggerPassSPC;
	GLuint m_debuggerPass_uni_normalTexture;
	GLuint m_debuggerPass_uni_p;
	GLuint m_debuggerPass_uni_r;	
	GLuint m_debuggerPass_uni_t;	
	GLuint m_debuggerPass_uni_m;

	GLRenderPassComponent* m_finalBlendPassGLRPC;
	GLShaderProgramComponent* m_finalBlendPassSPC;
	GLuint m_uni_motionBlurPassRT0;
	GLuint m_uni_skyPassRT0;
	GLuint m_uni_bloomPassRT0;
	GLuint m_uni_billboardPassRT0;
	GLuint m_uni_debuggerPassRT0;

private:
	GLFinalRenderPassSingletonComponent() {};
};

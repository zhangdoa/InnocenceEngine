#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

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

	GLFrameBufferComponent* m_skyPassFBC;
	GLShaderProgramComponent* m_skyPassSPC;
	GLuint m_skyPassVertexShaderID;
	GLuint m_skyPassFragmentShaderID;
	TextureDataComponent* m_skyPassTDC;
	GLTextureDataComponent* m_skyPassGLTDC;
	GLuint m_skyPass_uni_p;
	GLuint m_skyPass_uni_r;
	GLuint m_skyPass_uni_viewportSize;
	GLuint m_skyPass_uni_eyePos;	
	GLuint m_skyPass_uni_lightDir;

	GLFrameBufferComponent* m_TAAPingPassFBC;
	GLFrameBufferComponent* m_TAAPongPassFBC;
	GLShaderProgramComponent* m_TAAPassSPC;
	GLuint m_TAAPassVertexShaderID;
	GLuint m_TAAPassFragmentShaderID;
	TextureDataComponent* m_TAAPingPassTDC;
	GLTextureDataComponent* m_TAAPingPassGLTDC;
	TextureDataComponent* m_TAAPongPassTDC;
	GLTextureDataComponent* m_TAAPongPassGLTDC;
	GLuint m_TAAPass_uni_lightPassRT0;
	GLuint m_TAAPass_uni_lastTAAPassRT0;
	GLuint m_TAAPass_uni_motionVectorTexture;
	GLuint m_TAAPass_uni_renderTargetSize;

	GLFrameBufferComponent* m_bloomExtractPassFBC;
	GLShaderProgramComponent* m_bloomExtractPassSPC;
	GLuint m_bloomExtractPassVertexShaderID;
	GLuint m_bloomExtractPassFragmentShaderID;
	TextureDataComponent* m_bloomExtractPassTDC;
	GLTextureDataComponent* m_bloomExtractPassGLTDC;
	GLuint m_bloomExtractPass_uni_TAAPassRT0;

	GLFrameBufferComponent* m_bloomBlurPingPassFBC;
	GLFrameBufferComponent* m_bloomBlurPongPassFBC;
	GLShaderProgramComponent* m_bloomBlurPassSPC;
	GLuint m_bloomBlurPassVertexShaderID;
	GLuint m_bloomBlurPassFragmentShaderID;
	TextureDataComponent* m_bloomBlurPingPassTDC;
	GLTextureDataComponent* m_bloomBlurPingPassGLTDC;
	TextureDataComponent* m_bloomBlurPongPassTDC;
	GLTextureDataComponent* m_bloomBlurPongPassGLTDC;
	GLuint m_bloomBlurPass_uni_bloomExtractPassRT0;
	GLuint m_bloomBlurPass_uni_horizontal;

	GLFrameBufferComponent* m_motionBlurPassFBC;
	GLShaderProgramComponent* m_motionBlurPassSPC;
	GLuint m_motionBlurPassVertexShaderID;
	GLuint m_motionBlurPassFragmentShaderID;
	TextureDataComponent* m_motionBlurPassTDC;
	GLTextureDataComponent* m_motionBlurPassGLTDC;
	GLuint m_motionBlurPass_uni_motionVectorTexture;
	GLuint m_motionBlurPass_uni_TAAPassRT0;

	GLFrameBufferComponent* m_billboardPassFBC;
	GLShaderProgramComponent* m_billboardPassSPC;
	GLuint m_billboardPassVertexShaderID;
	GLuint m_billboardPassFragmentShaderID;
	TextureDataComponent* m_billboardPassTDC;
	GLTextureDataComponent* m_billboardPassGLTDC;
	GLuint m_billboardPass_uni_texture;
	GLuint m_billboardPass_uni_p;
	GLuint m_billboardPass_uni_r;
	GLuint m_billboardPass_uni_t;
	GLuint m_billboardPass_uni_pos;
	GLuint m_billboardPass_uni_albedo;
	GLuint m_billboardPass_uni_size;

	GLFrameBufferComponent* m_debuggerPassFBC;
	GLShaderProgramComponent* m_debuggerPassSPC;
	GLuint m_debuggerPassVertexShaderID;
	GLuint m_debuggerPassGeometryShaderID;
	GLuint m_debuggerPassFragmentShaderID;
	TextureDataComponent* m_debuggerPassTDC;
	GLTextureDataComponent* m_debuggerPassGLTDC;
	GLuint m_debuggerPass_uni_normalTexture;
	GLuint m_debuggerPass_uni_p;
	GLuint m_debuggerPass_uni_r;	
	GLuint m_debuggerPass_uni_t;	
	GLuint m_debuggerPass_uni_m;

	GLFrameBufferComponent* m_finalBlendPassFBC;
	GLShaderProgramComponent* m_finalBlendPassSPC;
	GLuint m_finalBlendPassVertexShaderID;
	GLuint m_finalBlendPassFragmentShaderID;
	TextureDataComponent* m_finalBlendPassTDC;
	GLTextureDataComponent* m_finalBlendPassGLTDC;
	GLuint m_uni_motionBlurPassRT0;
	GLuint m_uni_skyPassRT0;
	GLuint m_uni_bloomPassRT0;
	GLuint m_uni_billboardPassRT0;
	GLuint m_uni_debuggerPassRT0;

private:
	GLFinalRenderPassSingletonComponent() {};
};

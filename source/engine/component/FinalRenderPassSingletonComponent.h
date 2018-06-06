#pragma once
#include "BaseComponent.h"
#include "component/GLFrameBufferComponent.h"
#include "component/GLShaderProgramComponent.h"
#include "component/GLTextureDataComponent.h"

class FinalRenderPassSingletonComponent : public BaseComponent
{
public:
	~FinalRenderPassSingletonComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;
	
	static FinalRenderPassSingletonComponent& getInstance()
	{
		static FinalRenderPassSingletonComponent instance;
		return instance;
	}
	GLFrameBufferComponent m_skyPassGLFrameBufferComponent;
	GLShaderProgramComponent m_skyPassProgram;
	GLuint m_skyPassVertexShaderID;
	GLuint m_skyPassFragmentShaderID;
	GLTextureDataComponent m_skyPassTexture;
	GLuint m_uni_skybox;

	GLFrameBufferComponent m_debuggerPassGLFrameBufferComponent;
	GLShaderProgramComponent m_debuggerPassProgram;
	GLuint m_debuggerPassVertexShaderID;
	GLuint m_debuggerPassFragmentShaderID;
	GLTextureDataComponent m_debuggerPassTexture;

	GLFrameBufferComponent m_billboardPassGLFrameBufferComponent;
	GLShaderProgramComponent m_billboardPassProgram;
	GLuint m_billboardPassVertexShaderID;
	GLuint m_billboardPassFragmentShaderID;
	GLTextureDataComponent m_billboardPassTexture;

	GLFrameBufferComponent m_bloomExtractPassGLFrameBufferComponent;
	GLShaderProgramComponent m_bloomExtractPassProgram;
	GLuint m_bloomExtractPassVertexShaderID;
	GLuint m_bloomExtractPassFragmentShaderID;
	GLTextureDataComponent m_bloomExtractPassTexture;

	GLFrameBufferComponent m_bloomBlurPingPassGLFrameBufferComponent;
	GLFrameBufferComponent m_bloomBlurPongPassGLFrameBufferComponent;
	GLShaderProgramComponent m_bloomBlurPassProgram;
	GLuint m_bloomBlurPassVertexShaderID;
	GLuint m_bloomBlurPassFragmentShaderID;
	GLTextureDataComponent m_bloomBlurPingPassTexture;
	GLTextureDataComponent m_bloomBlurPongPassTexture;

	GLShaderProgramComponent m_finalBlendPassProgram;
	GLuint m_finalBlendPassVertexShaderID;
	GLuint m_finalBlendPassFragmentShaderID;
	GLuint m_uni_lightPassRT0;
	GLuint m_uni_skyPassRT0;
	GLuint m_uni_bloomPassRT0;
	GLuint m_uni_billboardPassRT0;
	GLuint m_uni_debuggerPassRT0;

private:
	FinalRenderPassSingletonComponent() {};
};

#pragma once
#include "../common/InnoType.h"
#include "GLShaderProgramComponent.h"
#include "GLRenderPassComponent.h"

class GLFinalRenderPassComponent
{
public:
	~GLFinalRenderPassComponent() {};

	static GLFinalRenderPassComponent& get()
	{
		static GLFinalRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLRenderPassComponent* m_skyPassGLRPC;
	GLShaderProgramComponent* m_skyPassGLSPC;
	ShaderFilePaths m_skyPassShaderFilePaths = { "GL4.0//skyPassVertex.sf", "", "GL4.0//skyPassFragment.sf" };

	GLuint m_skyPass_uni_p;
	GLuint m_skyPass_uni_r;
	GLuint m_skyPass_uni_viewportSize;
	GLuint m_skyPass_uni_eyePos;
	GLuint m_skyPass_uni_lightDir;

	GLRenderPassComponent* m_preTAAPassGLRPC;
	GLShaderProgramComponent* m_preTAAPassGLSPC;
	ShaderFilePaths m_preTAAPassShaderFilePaths = { "GL4.0//preTAAPassVertex.sf", "", "GL4.0//preTAAPassFragment.sf" };

	std::vector<std::string> m_preTAAPassUniformNames =
	{
		"uni_lightPassRT0",
		"uni_transparentPassRT0",
		"uni_transparentPassRT1",
		"uni_skyPassRT0",
		"uni_terrainPassRT0",
	};

	GLRenderPassComponent* m_TAAPingPassGLRPC;
	GLRenderPassComponent* m_TAAPongPassGLRPC;
	GLShaderProgramComponent* m_TAAPassGLSPC;
	ShaderFilePaths m_TAAPassShaderFilePaths = { "GL4.0//TAAPassVertex.sf", "", "GL4.0//TAAPassFragment.sf" };

	std::vector<std::string> m_TAAPassUniformNames =
	{
		"uni_preTAAPassRT0",
		"uni_lastFrameTAAPassRT0",
		"uni_motionVectorTexture",
	};

	GLRenderPassComponent* m_TAASharpenPassGLRPC;
	GLShaderProgramComponent* m_TAASharpenPassGLSPC;
	ShaderFilePaths m_TAASharpenPassShaderFilePaths = { "GL4.0//TAASharpenPassVertex.sf", "", "GL4.0//TAASharpenPassFragment.sf" };

	std::vector<std::string> m_TAASharpenPassUniformNames =
	{
		"uni_lastTAAPassRT0",
	};

	GLRenderPassComponent* m_bloomExtractPassGLRPC;
	GLShaderProgramComponent* m_bloomExtractPassGLSPC;
	ShaderFilePaths m_bloomExtractPassShaderFilePaths = { "GL4.0//bloomExtractPassVertex.sf", "", "GL4.0//bloomExtractPassFragment.sf" };

	std::vector<std::string> m_bloomExtractPassUniformNames =
	{
		"uni_TAAPassRT0",
	};

	GLRenderPassComponent* m_bloomDownsampleGLRPC_Half;
	GLRenderPassComponent* m_bloomDownsampleGLRPC_Quarter;
	GLRenderPassComponent* m_bloomDownsampleGLRPC_Eighth;

	GLRenderPassComponent* m_bloomBlurPingPassGLRPC;
	GLRenderPassComponent* m_bloomBlurPongPassGLRPC;
	GLShaderProgramComponent* m_bloomBlurPassGLSPC;
	ShaderFilePaths m_bloomBlurPassShaderFilePaths = { "GL4.0//bloomBlurPassVertex.sf", "", "GL4.0//bloomBlurPassFragment.sf" };

	std::vector<std::string> m_bloomBlurPassUniformNames =
	{
		"uni_bloomExtractPassRT0",
	};

	GLuint m_bloomBlurPass_uni_horizontal;

	GLRenderPassComponent* m_bloomMergePassGLRPC;
	GLShaderProgramComponent* m_bloomMergePassGLSPC;
	ShaderFilePaths m_bloomMergePassShaderFilePaths = { "GL4.0//bloomMergePassVertex.sf", "", "GL4.0//bloomMergePassFragment.sf" };

	std::vector<std::string> m_bloomMergePassUniformNames =
	{
		"uni_Full",
		"uni_Half",
		"uni_Quarter",
		"uni_Eighth",
	};

	GLRenderPassComponent* m_motionBlurPassGLRPC;
	GLShaderProgramComponent* m_motionBlurPassGLSPC;
	ShaderFilePaths m_motionBlurPassShaderFilePaths = { "GL4.0//motionBlurPassVertex.sf", "", "GL4.0//motionBlurPassFragment.sf" };

	std::vector<std::string> m_motionBlurPassUniformNames =
	{
		"uni_motionVectorTexture",
		"uni_TAAPassRT0",
	};

	GLRenderPassComponent* m_billboardPassGLRPC;
	GLShaderProgramComponent* m_billboardPassGLSPC;
	ShaderFilePaths m_billboardPassShaderFilePaths = { "GL4.0//billboardPassVertex.sf", "", "GL4.0//billboardPassFragment.sf" };

	std::vector<std::string> m_billboardPassUniformNames =
	{
		"uni_texture",
	};

	GLuint m_billboardPass_uni_p;
	GLuint m_billboardPass_uni_r;
	GLuint m_billboardPass_uni_t;
	GLuint m_billboardPass_uni_pos;
	GLuint m_billboardPass_uni_size;

	GLRenderPassComponent* m_debuggerPassGLRPC;
	GLShaderProgramComponent* m_debuggerPassGLSPC;
	ShaderFilePaths m_debuggerPassShaderFilePaths = { "GL4.0//wireframeOverlayPassVertex.sf", "", "GL4.0//wireframeOverlayPassFragment.sf" };
	//ShaderFilePaths m_debuggerPassShaderFilePaths = { "GL4.0//debuggerPassVertex.sf", "GL4.0//debuggerPassGeometry.sf", "GL4.0//debuggerPassFragment.sf" };

	std::vector<std::string> m_debuggerPassUniformNames =
	{
		"uni_normalTexture",
	};

	GLuint m_debuggerPass_uni_p;
	GLuint m_debuggerPass_uni_r;
	GLuint m_debuggerPass_uni_t;
	GLuint m_debuggerPass_uni_m;

	GLRenderPassComponent* m_finalBlendPassGLRPC;
	GLShaderProgramComponent* m_finalBlendPassGLSPC;
	ShaderFilePaths m_finalBlendPassShaderFilePaths = { "GL4.0//finalBlendPassVertex.sf", "", "GL4.0//finalBlendPassFragment.sf" };

	std::vector<std::string> m_finalBlendPassUniformNames =
	{
		"uni_basePassRT0",
		"uni_bloomPassRT0",
		"uni_billboardPassRT0",
		"uni_debuggerPassRT0",
	};

private:
	GLFinalRenderPassComponent() {};
};

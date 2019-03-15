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
	ShaderFilePaths m_skyPassShaderFilePaths = { "GL//skyPassVertex.sf", "", "GL//skyPassFragment.sf" };

	GLuint m_skyPass_uni_p;
	GLuint m_skyPass_uni_r;
	GLuint m_skyPass_uni_viewportSize;
	GLuint m_skyPass_uni_eyePos;
	GLuint m_skyPass_uni_lightDir;

	GLRenderPassComponent* m_preTAAPassGLRPC;
	GLShaderProgramComponent* m_preTAAPassGLSPC;
	ShaderFilePaths m_preTAAPassShaderFilePaths = { "GL//preTAAPassVertex.sf", "", "GL//preTAAPassFragment.sf" };

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
	ShaderFilePaths m_TAAPassShaderFilePaths = { "GL//TAAPassVertex.sf", "", "GL//TAAPassFragment.sf" };

	std::vector<std::string> m_TAAPassUniformNames =
	{
		"uni_preTAAPassRT0",
		"uni_lastFrameTAAPassRT0",
		"uni_motionVectorTexture",
	};

	GLRenderPassComponent* m_TAASharpenPassGLRPC;
	GLShaderProgramComponent* m_TAASharpenPassGLSPC;
	ShaderFilePaths m_TAASharpenPassShaderFilePaths = { "GL//TAASharpenPassVertex.sf", "", "GL//TAASharpenPassFragment.sf" };

	std::vector<std::string> m_TAASharpenPassUniformNames =
	{
		"uni_lastTAAPassRT0",
	};

	GLRenderPassComponent* m_bloomExtractPassGLRPC;
	GLShaderProgramComponent* m_bloomExtractPassGLSPC;
	ShaderFilePaths m_bloomExtractPassShaderFilePaths = { "GL//bloomExtractPassVertex.sf", "", "GL//bloomExtractPassFragment.sf" };

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
	ShaderFilePaths m_bloomBlurPassShaderFilePaths = { "GL//bloomBlurPassVertex.sf", "", "GL//bloomBlurPassFragment.sf" };

	std::vector<std::string> m_bloomBlurPassUniformNames =
	{
		"uni_bloomExtractPassRT0",
	};

	GLuint m_bloomBlurPass_uni_horizontal;

	GLRenderPassComponent* m_bloomMergePassGLRPC;
	GLShaderProgramComponent* m_bloomMergePassGLSPC;
	ShaderFilePaths m_bloomMergePassShaderFilePaths = { "GL//bloomMergePassVertex.sf", "", "GL//bloomMergePassFragment.sf" };

	std::vector<std::string> m_bloomMergePassUniformNames =
	{
		"uni_Full",
		"uni_Half",
		"uni_Quarter",
		"uni_Eighth",
	};

	GLRenderPassComponent* m_motionBlurPassGLRPC;
	GLShaderProgramComponent* m_motionBlurPassGLSPC;
	ShaderFilePaths m_motionBlurPassShaderFilePaths = { "GL//motionBlurPassVertex.sf", "", "GL//motionBlurPassFragment.sf" };

	std::vector<std::string> m_motionBlurPassUniformNames =
	{
		"uni_motionVectorTexture",
		"uni_TAAPassRT0",
	};

	GLRenderPassComponent* m_billboardPassGLRPC;
	GLShaderProgramComponent* m_billboardPassGLSPC;
	ShaderFilePaths m_billboardPassShaderFilePaths = { "GL//billboardPassVertex.sf", "", "GL//billboardPassFragment.sf" };

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
	ShaderFilePaths m_debuggerPassShaderFilePaths = { "GL//wireframeOverlayPassVertex.sf", "", "GL//wireframeOverlayPassFragment.sf" };
	//ShaderFilePaths m_debuggerPassShaderFilePaths = { "GL//debuggerPassVertex.sf", "GL//debuggerPassGeometry.sf", "GL//debuggerPassFragment.sf" };

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
	ShaderFilePaths m_finalBlendPassShaderFilePaths = { "GL//finalBlendPassVertex.sf", "", "GL//finalBlendPassFragment.sf" };

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

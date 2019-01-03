#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"
#include "GLRenderPassComponent.h"

class GLGeometryRenderPassComponent
{
public:
	~GLGeometryRenderPassComponent() {};

	static GLGeometryRenderPassComponent& get()
	{
		static GLGeometryRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLRenderPassComponent* m_opaquePass_GLRPC;

	GLShaderProgramComponent* m_opaquePass_GLSPC;

#ifdef CookTorrance
	ShaderFilePaths m_opaquePass_shaderFilePaths = { "GL4.0//opaquePassCookTorranceVertex.sf" , "", "GL4.0//opaquePassCookTorranceFragment.sf" };
#elif BlinnPhong
	ShaderFilePaths m_opaquePass_shaderFilePaths = { "GL4.0//opaquePassBlinnPhongVertex.sf" , "", "GL4.0//opaquePassBlinnPhongFragment.sf" };
#endif
	GLuint m_cameraUBO;
	GLuint m_meshUBO;

	GLuint m_textureUBO;

	std::vector<std::string> m_opaquePassTextureUniformNames =
	{
		"uni_normalTexture",
		"uni_albedoTexture",
		"uni_metallicTexture",
		"uni_roughnessTexture",
		"uni_aoTexture",
	};

	GLRenderPassComponent* m_SSAOPass_GLRPC;

	GLShaderProgramComponent* m_SSAOPass_GLSPC;

	ShaderFilePaths m_SSAOPass_shaderFilePaths = { "GL4.0//SSAOPassVertex.sf" , "", "GL4.0//SSAOPassFragment.sf" };

	GLuint m_SSAOPass_uni_p;
	GLuint m_SSAOPass_uni_r;
	GLuint m_SSAOPass_uni_t;

	std::vector<GLuint> m_SSAOPass_uni_samples;

	std::vector<std::string> m_SSAOPassTextureUniformNames =
	{
		"uni_Position",
		"uni_Normal",
		"uni_texNoise",
	};

	std::vector<vec4> ssaoKernel;
	std::vector<vec4> ssaoNoise;

	TextureDataComponent* m_noiseTDC;
	GLTextureDataComponent* m_noiseGLTDC;

	GLRenderPassComponent* m_SSAOBlurPass_GLRPC;

	GLShaderProgramComponent* m_SSAOBlurPass_GLSPC;

	ShaderFilePaths m_SSAOBlurPass_shaderFilePaths = { "GL4.0//SSAOBlurPassVertex.sf" , "", "GL4.0//SSAOBlurPassFragment.sf" };

	std::vector<std::string> m_SSAOBlurPassTextureUniformNames =
	{
		"uni_SSAOPassRT0",
	};

	GLRenderPassComponent* m_transparentPass_GLRPC;

	GLShaderProgramComponent* m_transparentPass_GLSPC;

	ShaderFilePaths m_transparentPass_shaderFilePaths = { "GL4.0//transparentPassVertex.sf" , "", "GL4.0//transparentPassFragment.sf" };

	GLuint m_transparentPass_uni_albedo;
	GLuint m_transparentPass_uni_viewPos;
	GLuint m_transparentPass_uni_dirLight_direction;
	GLuint m_transparentPass_uni_dirLight_color;
private:
	GLGeometryRenderPassComponent() {};
};

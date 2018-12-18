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
	GLuint m_lightUBO;

	GLuint m_textureUBO;

	std::vector<std::string> m_textureUniformNames =
	{
		"uni_normalTexture",
		"uni_albedoTexture",
		"uni_metallicTexture",
		"uni_roughnessTexture",
		"uni_aoTexture",
	};

	GLRenderPassComponent* m_transparentPass_GLRPC;

	GLShaderProgramComponent* m_transparentPass_GLSPC;

	ShaderFilePaths m_transparentPass_shaderFilePaths = { "GL4.0//transparentPassVertex.sf" , "", "GL4.0//transparentPassFragment.sf" };

	GLuint m_transparentPass_uni_albedo;
	GLuint m_transparentPass_uni_viewPos;

private:
	GLGeometryRenderPassComponent() {};
};

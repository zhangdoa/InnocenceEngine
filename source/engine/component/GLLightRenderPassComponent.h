#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

class GLLightRenderPassComponent
{
public:
	~GLLightRenderPassComponent() {};

	static GLLightRenderPassComponent& get()
	{
		static GLLightRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

#ifdef CookTorrance
	ShaderFilePaths m_shaderFilePaths = { "GL//lightPassCookTorranceVertex.sf" , "", "GL//lightPassCookTorranceFragment.sf" };
#elif BlinnPhong
	ShaderFilePaths m_shaderFilePaths = { "GL//lightPassBlinnPhongVertex.sf" , "", "GL//lightPassBlinnPhongFragment.sf" };
#endif
	std::vector<std::string> m_textureUniformNames =
	{
		"uni_opaquePassRT0",
		"uni_opaquePassRT1",
		"uni_opaquePassRT2",
		"uni_SSAOBlurPassRT0",
		"uni_directionalLightShadowMap",
		"uni_brdfLUT",
		"uni_brdfMSLUT",
		"uni_irradianceMap",
		"uni_preFiltedMap"
	};

	std::vector<GLuint> m_uni_shadowSplitAreas;
	std::vector<GLuint> m_uni_dirLightProjs;
	std::vector<GLuint> m_uni_dirLightViews;

	GLuint m_uni_viewPos;

	GLuint m_uni_dirLight_direction;
	GLuint m_uni_dirLight_luminance;
	GLuint m_uni_dirLight_rot;

	std::vector<GLuint> m_uni_pointLights_position;
	std::vector<GLuint> m_uni_pointLights_attenuationRadius;
	std::vector<GLuint> m_uni_pointLights_luminance;

	std::vector<GLuint> m_uni_sphereLights_position;
	std::vector<GLuint> m_uni_sphereLights_sphereRadius;
	std::vector<GLuint> m_uni_sphereLights_luminance;

	GLuint m_uni_isEmissive;
private:
	GLLightRenderPassComponent() {};
};

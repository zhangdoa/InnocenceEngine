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

	GLuint m_opaquePass_uni_p_camera_original;
	GLuint m_opaquePass_uni_p_camera_jittered;
	GLuint m_opaquePass_uni_r_camera;
	GLuint m_opaquePass_uni_t_camera;
	GLuint m_opaquePass_uni_r_camera_prev;
	GLuint m_opaquePass_uni_t_camera_prev;
	GLuint m_opaquePass_uni_m;
	GLuint m_opaquePass_uni_m_prev;

	GLuint m_opaquePass_uni_p_light_0;
	GLuint m_opaquePass_uni_p_light_1;
	GLuint m_opaquePass_uni_p_light_2;
	GLuint m_opaquePass_uni_p_light_3;
	GLuint m_opaquePass_uni_v_light;

	GLuint m_opaquePass_uni_normalTexture;
	GLuint m_opaquePass_uni_albedoTexture;
	GLuint m_opaquePass_uni_metallicTexture;
	GLuint m_opaquePass_uni_roughnessTexture;
	GLuint m_opaquePass_uni_aoTexture;

	GLuint m_opaquePass_uni_useNormalTexture;
	GLuint m_opaquePass_uni_useAlbedoTexture;
	GLuint m_opaquePass_uni_useMetallicTexture;
	GLuint m_opaquePass_uni_useRoughnessTexture;
	GLuint m_opaquePass_uni_useAOTexture;

	GLuint m_opaquePass_uni_albedo;
	GLuint m_opaquePass_uni_MRA;

	GLRenderPassComponent* m_transparentPass_GLRPC;

	GLShaderProgramComponent* m_transparentPass_GLSPC;

	ShaderFilePaths m_transparentPass_shaderFilePaths = { "GL4.0//transparentPassVertex.sf" , "", "GL4.0//transparentPassFragment.sf" };

	GLuint m_transparentPass_uni_p_camera_original;
	GLuint m_transparentPass_uni_p_camera_jittered;
	GLuint m_transparentPass_uni_r_camera;
	GLuint m_transparentPass_uni_t_camera;
	GLuint m_transparentPass_uni_r_camera_prev;
	GLuint m_transparentPass_uni_t_camera_prev;
	GLuint m_transparentPass_uni_m;
	GLuint m_transparentPass_uni_m_prev;

	GLuint m_transparentPass_uni_albedo;
	GLuint m_transparentPass_uni_viewPos;

private:
	GLGeometryRenderPassComponent() {};
};

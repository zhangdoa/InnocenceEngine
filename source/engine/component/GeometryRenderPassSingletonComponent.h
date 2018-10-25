#pragma once
#include "BaseComponent.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "GLTextureDataComponent.h"

class GeometryRenderPassSingletonComponent : public BaseComponent
{
public:
	~GeometryRenderPassSingletonComponent() {};
	
	static GeometryRenderPassSingletonComponent& getInstance()
	{
		static GeometryRenderPassSingletonComponent instance;
		return instance;
	}

	GLFrameBufferComponent* m_FBC;

	GLShaderProgramComponent* m_GLSPC;
	GLuint m_geometryPassVertexShaderID;
	GLuint m_geometryPassFragmentShaderID;
	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<GLTextureDataComponent*> m_GLTDCs;

	GLuint m_geometryPass_uni_p_camera_original;
	GLuint m_geometryPass_uni_p_camera_jittered;
	GLuint m_geometryPass_uni_r_camera;
	GLuint m_geometryPass_uni_t_camera;
	GLuint m_geometryPass_uni_r_camera_prev;
	GLuint m_geometryPass_uni_t_camera_prev;
	GLuint m_geometryPass_uni_m;
	GLuint m_geometryPass_uni_m_prev;

	GLuint m_geometryPass_uni_p_light_0;
	GLuint m_geometryPass_uni_p_light_1;
	GLuint m_geometryPass_uni_p_light_2;
	GLuint m_geometryPass_uni_p_light_3;
	GLuint m_geometryPass_uni_v_light;

	GLuint m_geometryPass_uni_normalTexture;
	GLuint m_geometryPass_uni_albedoTexture;
	GLuint m_geometryPass_uni_metallicTexture;
	GLuint m_geometryPass_uni_roughnessTexture;
	GLuint m_geometryPass_uni_aoTexture;
	GLuint m_geometryPass_uni_useTexture;
	GLuint m_geometryPass_uni_albedo;
	GLuint m_geometryPass_uni_MRA;
private:
	GeometryRenderPassSingletonComponent() {};
};

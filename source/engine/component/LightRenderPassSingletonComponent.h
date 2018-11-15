#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

class LightRenderPassSingletonComponent
{
public:
	~LightRenderPassSingletonComponent() {};
	
	static LightRenderPassSingletonComponent& getInstance()
	{
		static LightRenderPassSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFrameBufferComponent* m_FBC;

	GLShaderProgramComponent* m_GLSPC;
	GLuint m_lightPassVertexShaderID;
	GLuint m_lightPassFragmentShaderID;
	TextureDataComponent* m_TDC;
	GLTextureDataComponent* m_GLTDC;

	GLuint m_uni_geometryPassRT0;
	GLuint m_uni_geometryPassRT1;
	GLuint m_uni_geometryPassRT2;

	GLuint m_uni_geometryPassRT4;
	GLuint m_uni_geometryPassRT5;
	GLuint m_uni_geometryPassRT6;
	GLuint m_uni_geometryPassRT7;

	GLuint m_uni_shadowMap_0;
	GLuint m_uni_shadowMap_1;
	GLuint m_uni_shadowMap_2;
	GLuint m_uni_shadowMap_3;
	std::vector<GLuint> m_uni_shadowSplitAreas;

	GLuint m_uni_irradianceMap;
	GLuint m_uni_preFiltedMap;
	GLuint m_uni_brdfLUT;
	GLuint m_uni_viewPos;

	GLuint m_uni_dirLight_position;
	GLuint m_uni_dirLight_direction;
	GLuint m_uni_dirLight_color;
	std::vector<GLuint> m_uni_pointLights_position;
	std::vector<GLuint> m_uni_pointLights_radius;
	std::vector<GLuint> m_uni_pointLights_color;

	GLuint m_uni_isEmissive;
private:
	LightRenderPassSingletonComponent() {};
};

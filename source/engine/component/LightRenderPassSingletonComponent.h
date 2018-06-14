#pragma once
#include "BaseComponent.h"
#include "component/GLFrameBufferComponent.h"
#include "component/GLShaderProgramComponent.h"
#include "component/GLTextureDataComponent.h"

class LightRenderPassSingletonComponent : public BaseComponent
{
public:
	~LightRenderPassSingletonComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;
	
	static LightRenderPassSingletonComponent& getInstance()
	{
		static LightRenderPassSingletonComponent instance;
		return instance;
	}

	GLFrameBufferComponent m_GLFrameBufferComponent;

	GLShaderProgramComponent m_lightPassProgram;
	GLuint m_lightPassVertexShaderID;
	GLuint m_lightPassFragmentShaderID;
	GLTextureDataComponent m_lightPassTexture;

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

	GLuint m_uni_irradianceMap;
	GLuint m_uni_preFiltedMap;
	GLuint m_uni_brdfLUT;
	GLuint m_uni_viewPos;

	GLuint  m_uni_dirLight_position;
	GLuint  m_uni_dirLight_direction;
	GLuint  m_uni_dirLight_color;
	std::vector<GLuint> m_uni_pointLights_position;
	std::vector<GLuint> m_uni_pointLights_radius;
	std::vector<GLuint> m_uni_pointLights_color;
private:
	LightRenderPassSingletonComponent() {};
};

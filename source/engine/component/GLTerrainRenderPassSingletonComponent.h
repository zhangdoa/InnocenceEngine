#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"
#include "GLRenderPassComponent.h"

class GLTerrainRenderPassSingletonComponent
{
public:
	~GLTerrainRenderPassSingletonComponent() {};
	
	static GLTerrainRenderPassSingletonComponent& getInstance()
	{
		static GLTerrainRenderPassSingletonComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;
	
	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	GLuint m_terrainPass_uni_p_camera;
	GLuint m_terrainPass_uni_r_camera;
	GLuint m_terrainPass_uni_t_camera;
	GLuint m_terrainPass_uni_m;

private:
	GLTerrainRenderPassSingletonComponent() {};
};

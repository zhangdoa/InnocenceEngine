#pragma once
#include "BaseComponent.h"
#include "component/GLFrameBufferComponent.h"
#include "component/GLShaderProgramComponent.h"
#include "component/GLTextureDataComponent.h"

class GeometryRenderPassSingletonComponent : public BaseComponent
{
public:
	~GeometryRenderPassSingletonComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;
	
	static GeometryRenderPassSingletonComponent& getInstance()
	{
		static GeometryRenderPassSingletonComponent instance;
		return instance;
	}

	GLFrameBufferComponent m_GLFrameBufferComponent;

	GLShaderProgramComponent m_geometryPassProgram;
	GLuint m_geometryPassVertexShaderID;
	GLuint m_geometryPassFragmentShaderID;
	GLTextureDataComponent m_geometryPassTextureID_RT0;
	GLTextureDataComponent m_geometryPassTextureID_RT1;
	GLTextureDataComponent m_geometryPassTextureID_RT2;
	GLTextureDataComponent m_geometryPassTextureID_RT3;
	GLuint m_geometryPass_uni_p;
	GLuint m_geometryPass_uni_r;
	GLuint m_geometryPass_uni_t;
	GLuint m_geometryPass_uni_m;
private:
	GeometryRenderPassSingletonComponent() {};
};

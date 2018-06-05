#pragma once
#include "BaseComponent.h"
#include "component/GLFrameBufferComponent.h"
#include "component/GLShaderProgramComponent.h"
#include "component/GLTextureDataComponent.h"

class ShadowRenderPassSingletonComponent : public BaseComponent
{
public:
	~ShadowRenderPassSingletonComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;
	
	static ShadowRenderPassSingletonComponent& getInstance()
	{
		static ShadowRenderPassSingletonComponent instance;
		return instance;
	}

	GLFrameBufferComponent m_GLFrameBufferComponent;

	GLShaderProgramComponent m_shadowPassProgram;
	GLuint m_shadowPassVertexShaderID;
	GLuint m_shadowPassFragmentShaderID;
	GLTextureDataComponent m_shadowForwardPassTextureID_L0;
	GLTextureDataComponent m_shadowForwardPassTextureID_L1;
	GLTextureDataComponent m_shadowForwardPassTextureID_L2;
	GLTextureDataComponent m_shadowForwardPassTextureID_L3;
	GLuint m_shadowPass_uni_p;
	GLuint m_shadowPass_uni_v;
	GLuint m_shadowPass_uni_m;
private:
	ShadowRenderPassSingletonComponent() {};
};

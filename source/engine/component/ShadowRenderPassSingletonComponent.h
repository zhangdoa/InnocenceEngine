#pragma once
#include "BaseComponent.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "GLTextureDataComponent.h"

class ShadowRenderPassSingletonComponent : public BaseComponent
{
public:
	~ShadowRenderPassSingletonComponent() {};

	static ShadowRenderPassSingletonComponent& getInstance()
	{
		static ShadowRenderPassSingletonComponent instance;
		return instance;
	}

	std::vector<std::pair<GLFrameBufferComponent, GLTextureDataComponent>> m_frameBufferTextureVector;

	GLShaderProgramComponent m_shadowPassProgram;
	GLuint m_shadowPassVertexShaderID;
	GLuint m_shadowPassFragmentShaderID;
	GLuint m_shadowPass_uni_p;
	GLuint m_shadowPass_uni_v;
	GLuint m_shadowPass_uni_m;
private:
	ShadowRenderPassSingletonComponent() {};
};

#pragma once
#include "../common/InnoType.h"
#include "GLRenderPassComponent.h"
#include "GLShaderProgramComponent.h"

class GLShadowRenderPassSingletonComponent
{
public:
	~GLShadowRenderPassSingletonComponent() {};

	static GLShadowRenderPassSingletonComponent& getInstance()
	{
		static GLShadowRenderPassSingletonComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<GLRenderPassComponent*> m_GLRPCs;

	GLShaderProgramComponent* m_SPC;

	GLuint m_shadowPass_uni_p;
	GLuint m_shadowPass_uni_v;
	GLuint m_shadowPass_uni_m;
private:
	GLShadowRenderPassSingletonComponent() {};
};

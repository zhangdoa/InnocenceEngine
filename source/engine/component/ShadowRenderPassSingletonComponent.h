#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

class ShadowRenderPassSingletonComponent
{
public:
	~ShadowRenderPassSingletonComponent() {};

	static ShadowRenderPassSingletonComponent& getInstance()
	{
		static ShadowRenderPassSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<GLFrameBufferComponent*> m_FBCs;
	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<GLTextureDataComponent*> m_GLTDCs;

	GLShaderProgramComponent* m_SPC;

	GLuint m_shadowPass_uni_p;
	GLuint m_shadowPass_uni_v;
	GLuint m_shadowPass_uni_m;
private:
	ShadowRenderPassSingletonComponent() {};
};

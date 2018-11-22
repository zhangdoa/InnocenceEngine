#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"
#include "GLFrameBufferComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

class GLRenderPassComponent
{
public:
	GLRenderPassComponent() {};
	~GLRenderPassComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	GLFrameBufferComponent* m_GLFBC = 0;
	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<GLTextureDataComponent*> m_GLTDCs;
};
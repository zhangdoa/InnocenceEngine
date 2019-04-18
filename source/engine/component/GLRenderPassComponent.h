#pragma once
#include "../common/InnoType.h"
#include "../system/GLRenderingBackend/GLHeaders.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

struct GLFrameBufferDesc
{
	GLenum renderBufferAttachmentType = 0;
	GLenum renderBufferInternalFormat = 0;
	GLsizei sizeX = 0;
	GLsizei sizeY = 0;
	bool drawColorBuffers = true;
};

class GLRenderPassComponent
{
public:
	GLRenderPassComponent() {};
	~GLRenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_FBO = 0;
	GLuint m_RBO = 0;

	GLFrameBufferDesc m_GLFrameBufferDesc;
	std::vector<GLTextureDataComponent*> m_GLTDCs;
};
#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"
#include "RenderPassComponent.h"
#include "GLTextureDataComponent.h"

class GLRenderPassComponent : public RenderPassComponent
{
public:
	GLRenderPassComponent() {};
	~GLRenderPassComponent() {};

	GLuint m_FBO = 0;
	GLuint m_RBO = 0;

	GLenum m_renderBufferAttachmentType = 0;
	GLenum m_renderBufferInternalFormat = 0;
	bool m_drawColorBuffers = true;

	std::vector<GLTextureDataComponent*> m_GLTDCs;
};
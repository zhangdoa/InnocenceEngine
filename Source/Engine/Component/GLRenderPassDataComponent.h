#pragma once
#include "RenderPassDataComponent.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"

class GLRenderPassDataComponent : public RenderPassDataComponent
{
public:
	GLRenderPassDataComponent() {};
	~GLRenderPassDataComponent() {};

	GLuint m_FBO = 0;
	GLuint m_RBO = 0;

	GLenum m_renderBufferAttachmentType = 0;
	GLenum m_renderBufferInternalFormat = 0;
};
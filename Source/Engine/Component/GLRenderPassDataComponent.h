#pragma once
#include "RenderPassDataComponent.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"

class GLPipelineStateObject : public IPipelineStateObject
{
public:
	std::deque<std::function<void()>> m_Activate;
	std::deque<std::function<void()>> m_Deactivate;
	GLenum m_GLPrimitiveTopology = 0;
};

class GLResourceBinder : public IResourceBinder
{
public:
	void* m_TO = 0;
	GLuint m_SO = 0;
	GLuint m_BO = 0;
};

class GLCommandList : public ICommandList
{
};

class GLCommandQueue : public ICommandQueue
{
};

class GLSemaphore : public ISemaphore
{
};

class GLFence : public IFence
{
};

class GLRenderPassDataComponent : public RenderPassDataComponent
{
public:
	GLuint m_FBO = 0;
	GLuint m_RBO = 0;

	GLenum m_renderBufferAttachmentType = 0;
	GLenum m_renderBufferInternalFormat = 0;
};
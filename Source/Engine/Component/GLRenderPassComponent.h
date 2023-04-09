#pragma once
#include "RenderPassComponent.h"
#include "../RenderingServer/GL/GLHeaders.h"

namespace Inno
{
	class GLPipelineStateObject : public IPipelineStateObject
	{
	public:
		std::deque<std::function<void()>> m_Activate;
		std::deque<std::function<void()>> m_Deactivate;
		GLenum m_GLPrimitiveTopology = 0;
	};

	class GLCommandList : public ICommandList
	{
	};

	class GLSemaphore : public ISemaphore
	{
	};

	class GLRenderPassComponent : public RenderPassComponent
	{
	public:
		GLuint m_FBO = 0;
		GLuint m_RBO = 0;

		GLenum m_renderBufferAttachmentType = 0;
		GLenum m_renderBufferInternalFormat = 0;
	};
}
#include "GLFrameBuffer.h"

void GLFrameBuffer::initialize()
{
	//generate and bind frame buffer
	glGenFramebuffers(1, &m_FBO[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[0]);

	//if (m_frameBufferType == frameBufferType::PINGPONG)
	//{
	//	glGenFramebuffers(1, &m_FBO[1]);
	//	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[1]);
	//}

	if (m_renderBufferType != renderBufferType::NONE)
	{
		// generate and bind render buffer
		glGenRenderbuffers(1, &m_RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);

		switch (m_renderBufferType)
		{
		case renderBufferType::DEPTH:m_internalformat = GL_DEPTH_COMPONENT32; m_attachment = GL_DEPTH_ATTACHMENT; break;
		case renderBufferType::STENCIL:m_internalformat = GL_STENCIL_INDEX16; m_attachment = GL_STENCIL_ATTACHMENT; break;
		case renderBufferType::DEPTH_AND_STENCIL: m_internalformat = GL_DEPTH24_STENCIL8; m_attachment = GL_DEPTH_STENCIL_ATTACHMENT; break;
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_attachment, GL_RENDERBUFFER, m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, m_internalformat, (int)m_renderTargetTextures[0]->getTextureWidth(), (int)m_renderTargetTextures[0]->getTextureHeight());
	}
	
	if (m_renderTargetTextures.size() > 0)
	{
		if (m_frameBufferType != frameBufferType::ENVIRONMENT_PASS)
		{
			for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); ++i)
			{
				m_renderTargetTextures[i]->initialize();
				m_renderTargetTextures[i]->attachToFramebuffer(i, 0, 0);
			}
			std::vector<unsigned int> colorAttachments;
			for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); ++i)
			{
				colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
			}
			glDrawBuffers(colorAttachments.size(), &colorAttachments[0]);
		}
		else if (m_frameBufferType == frameBufferType::SHADOW_PASS)
		{
			for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); ++i)
			{
				m_renderTargetTextures[i]->initialize();
				m_renderTargetTextures[i]->attachToFramebuffer(0, 0, 0);
			}
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		else
		{
			for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); ++i)
			{
				m_renderTargetTextures[i]->initialize();
			}
		}
	}
	else
	{
		g_pLogSystem->printLog("GLFrameBuffer: Error: No render target textures added!");
		return;
	}

	for (auto i = (unsigned int)0; i < m_shaderPrograms.size(); ++i)
	{
		m_shaderPrograms[i]->initialize();
	}

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap, bool cleanColorBuffer, bool cleanDepthBuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[0]);
	if (m_renderBufferType != renderBufferType::NONE)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
	}
	if (cleanColorBuffer)
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}
	if (cleanDepthBuffer)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

void GLFrameBuffer::setRenderBufferStorageSize(std::vector<CameraComponent *> & cameraComponents, std::vector<LightComponent *> & lightComponents, std::vector<VisibleComponent *> & visibleComponents, std::unordered_map<EntityID, BaseMesh *> & meshMap, std::unordered_map<EntityID, BaseTexture *> & textureMap)
{
	for (auto i = (unsigned int)0; i < m_shaderPrograms.size(); ++i)
	{
		if (m_renderBufferType != renderBufferType::NONE)
		{
			glRenderbufferStorage(GL_RENDERBUFFER, m_internalformat, (int)m_renderBufferStorageSize[i].x, (int)m_renderBufferStorageSize[i].y);
			glViewport(0, 0, (int)m_renderBufferStorageSize[i].x, (int)m_renderBufferStorageSize[i].y);
		}
		m_shaderPrograms[i]->update(cameraComponents, lightComponents, visibleComponents, meshMap, textureMap);
	}
}

void GLFrameBuffer::activeTexture(int textureIndexInOwnerFrameBuffer, int textureIndexInUserFrameBuffer)
{
	m_renderTargetTextures[textureIndexInOwnerFrameBuffer]->update(textureIndexInUserFrameBuffer);
}

void GLFrameBuffer::asReadBuffer()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO[0]);
}

void GLFrameBuffer::asWriteBuffer(const vec2& source, const vec2& dest)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO[0]);
	glBlitFramebuffer(0, 0, (GLint)source.x, (GLint)source.y, 0, 0, dest.x, dest.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void GLFrameBuffer::shutdown()
{
	for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); ++i)
	{
		m_renderTargetTextures[i]->shutdown();
	}
	glDeleteFramebuffers(1, &m_FBO[0]);
	glDeleteRenderbuffers(1, &m_RBO);
}
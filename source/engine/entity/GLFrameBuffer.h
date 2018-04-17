#pragma once
#include "common/GLHeaders.h"
#include "BaseFrameBuffer.h"
#include "ComponentHeaders.h"

class GLFrameBuffer : public BaseFrameBuffer
{
public:
	GLFrameBuffer() {};
	virtual ~GLFrameBuffer() {};

	void initialize() override;
	void update(bool cleanColorBuffer, bool cleanDepthBuffer) override;
	void setRenderBufferStorageSize(unsigned int RenderBufferTextureIndex) override;
	void activeTexture(int textureIndexInOwnerFrameBuffer, int textureIndexInUserFrameBuffer) override;
	void asReadBuffer() override;
	void asWriteBuffer(const vec2& source, const vec2& dest) override;
	void shutdown() override;

private:
	GLuint m_FBO;
	GLuint m_RBO;
	GLenum m_internalformat;
	GLenum m_attachment;
};
#pragma once
#include "common/GLHeaders.h"
#include "BaseTexture.h"

class GLTexture : public BaseTexture
{
public:
	GLTexture() {};
	~GLTexture() {};

	void initialize() override;
	void activate(int textureIndex) override;
	void shutdown() override;
	void attachToFramebuffer(int colorAttachmentIndex, int textureIndex, int mipLevel) override;

private:
	GLuint m_textureID = 0;
};
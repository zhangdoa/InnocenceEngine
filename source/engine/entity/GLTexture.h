#pragma once
#include "BaseTexture.h"

class GLTexture : public BaseTexture
{
public:
	GLTexture() {};
	~GLTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex) override;
	void shutdown() override;
	void attachToFramebuffer(int colorAttachmentIndex, int textureIndex, int mipLevel) override;

private:
	GLuint m_textureID = 0;
};
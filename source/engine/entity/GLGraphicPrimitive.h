#pragma once
#include "interface/ILogSystem.h"
#include "entity/BaseGraphicPrimitive.h"

extern ILogSystem* g_pLogSystem;

class GLMesh : public BaseMesh
{
public:
	GLMesh() {};
	~GLMesh() {};

	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

class GLTexture : public BaseTexture
{
public:
	GLTexture() {};
	~GLTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex) override;
	void shutdown() override;
	void updateFramebuffer(int colorAttachmentIndex, int textureIndex, int mipLevel) override;
private:
	GLuint m_textureID = 0;
};

class GLFrameBuffer : public BaseFrameBuffer
{
public:
	GLFrameBuffer() {};
	virtual ~GLFrameBuffer() {};

	void initialize() override;
	void update() override;
	void activeTexture(int textureLevel, int textureIndex);
	void shutdown() override;

private:
	GLuint m_FBO;
	GLuint m_RBO;

	std::vector<GLuint> m_textures;
};
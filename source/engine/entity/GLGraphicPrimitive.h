#pragma once
#include "interface/ILogSystem.h"
#include "entity/BaseGraphicPrimitive.h"

extern ILogSystem* g_pLogSystem;

class GL2DMesh : public BaseMesh
{
public:
	GL2DMesh() {};
	~GL2DMesh() {};

	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

class GL3DMesh : public BaseMesh
{
public:
	GL3DMesh() {};
	~GL3DMesh() {};

	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

class GL2DTexture : public BaseTexture
{
public:
	GL2DTexture() {};
	~GL2DTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex);
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};

class GL2DHDRTexture : public BaseTexture
{
public:
	GL2DHDRTexture() {};
	~GL2DHDRTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex);
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};

class GL3DTexture : public BaseTexture
{
public:
	GL3DTexture() {};
	~GL3DTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex);
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};

class GL3DHDRTexture : public BaseTexture
{
public:
	GL3DHDRTexture() {};
	~GL3DHDRTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex);
	void shutdown() override;

	void updateFramebuffer(int index, int mipLevel);

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
	void drawMesh();
	void shutdown() override;

private:
	GLuint m_FBO;
	GLuint m_RBO;

	std::vector<GLuint> m_textures;

	GLuint m_VAO;
	GLuint m_VBO;
	std::vector<float> m_Vertices;
};
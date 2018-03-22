#pragma once
#include "entity/BaseGraphicPrimitive.h"

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

class GLShader : public BaseShader
{
public:
	GLShader();
	~GLShader();

	void initialize() override;
	void update() override;
	void shutdown() override;

	const GLint & getShaderID() const;

private:
	GLint m_shaderID;
};


class GLShaderProgram : public BaseShaderProgram
{
public:
	GLShaderProgram();
	virtual ~GLShaderProgram();

	void initialize() override;
	void shutdown() override;

protected:
	void attachShader(const GLShader* GLShader) const;
	void setAttributeLocation(int arrtributeLocation, const std::string& arrtributeName) const;

	inline void useProgram() const;

	inline void addUniform(std::string uniform) const;
	inline GLint getUniformLocation(const std::string &uniformName) const;

	inline void updateUniform(const GLint uniformLocation, bool uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, int uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z, double w);
	inline void updateUniform(const GLint uniformLocation, const mat4& mat) const;

	GLShader m_vertexShader;
	GLShader m_geometryShader;
	GLShader m_fragmentShader;

	unsigned int m_program;
};

class GLFrameBufferWIP : public BaseFrameBufferWIP
{
public:
	GLFrameBufferWIP() {};
	virtual ~GLFrameBufferWIP() {};

	void initialize() override;
	void update() override;
	void activeTexture(int colorAttachmentIndex, int textureIndex, int textureMipMapLevel) override;
	void shutdown() override;

private:
	GLuint m_FBO;
	GLuint m_RBO;
	GLenum m_internalformat;
	GLenum m_attachment;
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
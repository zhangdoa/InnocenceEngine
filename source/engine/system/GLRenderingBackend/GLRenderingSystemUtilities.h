#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoClassTemplate.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/GLMeshDataComponent.h"
#include "../../component/GLTextureDataComponent.h"
#include "../../component/GLRenderPassComponent.h"
#include "../../component/GLShaderProgramComponent.h"

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	bool initializeComponentPool();

	GLRenderPassComponent* addGLRenderPassComponent(unsigned int RTNum, GLFrameBufferDesc glFrameBufferDesc, TextureDataDesc RTDesc);

	bool resizeGLRenderPassComponent(GLRenderPassComponent * GLRPC, unsigned int newSizeX, unsigned int newSizeY);

	GLMeshDataComponent* generateGLMeshDataComponent(MeshDataComponent* rhs);
	GLTextureDataComponent* generateGLTextureDataComponent(TextureDataComponent* rhs);

	GLMeshDataComponent* addGLMeshDataComponent(const EntityID& rhs);
	GLTextureDataComponent* addGLTextureDataComponent(const EntityID& rhs);

	GLMeshDataComponent* getGLMeshDataComponent(const EntityID& rhs);
	GLTextureDataComponent* getGLTextureDataComponent(const EntityID& rhs);

	GLShaderProgramComponent* addGLShaderProgramComponent(const EntityID& rhs);

	bool initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool deleteShaderProgram(GLShaderProgramComponent* rhs);

	GLuint getUniformLocation(GLuint shaderProgram, const std::string& uniformName);
	GLuint getUniformBlockIndex(GLuint shaderProgram, const std::string& uniformBlockName);
	GLuint generateUBO(GLuint UBOSize);
	void bindUniformBlock(GLuint UBO, GLuint UBOSize, GLuint program, const std::string & uniformBlockName, GLuint uniformBlockBindingPoint);
	void updateTextureUniformLocations(GLuint program, const std::vector<std::string>& UniformNames);

	void updateUBOImpl(const GLint& UBO, size_t size, const void* UBOValue);

	template<typename T>
	void updateUBO(const GLint& UBO, const T& UBOValue)
	{
		updateUBOImpl(UBO, sizeof(T), &UBOValue);
	}

	void updateUniform(const GLint uniformLocation, bool uniformValue);
	void updateUniform(const GLint uniformLocation, int uniformValue);
	void updateUniform(const GLint uniformLocation, unsigned int uniformValue);
	void updateUniform(const GLint uniformLocation, float uniformValue);
	void updateUniform(const GLint uniformLocation, float x, float y);
	void updateUniform(const GLint uniformLocation, float x, float y, float z);
	void updateUniform(const GLint uniformLocation, float x, float y, float z, float w);
	void updateUniform(const GLint uniformLocation, const mat4& mat);
	void updateUniform(const GLint uniformLocation, const std::vector<vec4>& uniformValue);

	void attach2DDepthRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC);
	void attachCubemapDepthRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int textureIndex, unsigned int mipLevel);
	void attach2DColorRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex);
	void attach3DColorRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int layer);
	void attachCubemapColorRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel);
	void activateShaderProgram(GLShaderProgramComponent* GLShaderProgramComponent);

	void drawMesh(const EntityID& rhs);
	void drawMesh(MeshDataComponent* MDC);
	void drawMesh(size_t indicesSize, MeshPrimitiveTopology MeshPrimitiveTopology, GLMeshDataComponent* GLMDC);
	void activateTexture(TextureDataComponent* TDC, int activateIndex);
	void activateTexture(GLTextureDataComponent* GLTDC, int activateIndex);

	void activateRenderPass(GLRenderPassComponent * GLRPC);
	void cleanRenderBuffers(GLRenderPassComponent * GLRPC);
	void copyDepthBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest);
	void copyStencilBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest);
	void copyColorBuffer(GLRenderPassComponent* src, unsigned int srcIndex, GLRenderPassComponent* dest, unsigned int destIndex);

	vec4 readPixel(GLRenderPassComponent* GLRPC, unsigned int colorAttachmentIndex, GLint x, GLint y);
}

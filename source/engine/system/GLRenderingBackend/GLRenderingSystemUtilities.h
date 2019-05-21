#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/MaterialDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/GLMeshDataComponent.h"
#include "../../component/GLTextureDataComponent.h"
#include "../../component/GLRenderPassComponent.h"
#include "../../component/GLShaderProgramComponent.h"

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool render();
	bool terminate();
	bool initializeComponentPool();
	bool resize();

	void loadDefaultAssets();
	bool generateGPUBuffers();

	GLMeshDataComponent* addGLMeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	GLTextureDataComponent* addGLTextureDataComponent();

	GLMeshDataComponent* getGLMeshDataComponent(MeshShapeType MeshShapeType);
	GLTextureDataComponent* getGLTextureDataComponent(TextureUsageType TextureUsageType);
	GLTextureDataComponent* getGLTextureDataComponent(FileExplorerIconType iconType);
	GLTextureDataComponent* getGLTextureDataComponent(WorldEditorIconType iconType);

	GLRenderPassComponent* addGLRenderPassComponent(const EntityID& parentEntity, const char* name);
	bool initializeGLRenderPassComponent(GLRenderPassComponent* GLRPC);

	bool resizeGLRenderPassComponent(GLRenderPassComponent * GLRPC, unsigned int newSizeX, unsigned int newSizeY);

	bool initializeGLMeshDataComponent(GLMeshDataComponent* rhs);
	bool initializeGLTextureDataComponent(GLTextureDataComponent* rhs);

	GLShaderProgramComponent* addGLShaderProgramComponent(const EntityID& rhs);

	bool initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool deleteShaderProgram(GLShaderProgramComponent* rhs);

	GLuint getUniformLocation(GLuint shaderProgram, const std::string& uniformName);
	GLuint generateUBO(GLuint UBOSize, GLuint uniformBlockBindingPoint, const std::string& UBOName);

	void updateUBOImpl(const GLint& UBO, size_t size, const void* UBOValue);

	template<typename T>
	void updateUBO(const GLint& UBO, const T& UBOValue)
	{
		updateUBOImpl(UBO, sizeof(T), &UBOValue);
	}

	template<typename T>
	void updateUBO(const GLint& UBO, const std::vector<T>& UBOValue)
	{
		updateUBOImpl(UBO, sizeof(T) * UBOValue.size(), &UBOValue[0]);
	}

	bool bindUBO(const GLint& UBO, GLuint uniformBlockBindingPoint, unsigned int offset, unsigned int size);

	void updateUniform(const GLint uniformLocation, bool uniformValue);
	void updateUniform(const GLint uniformLocation, int uniformValue);
	void updateUniform(const GLint uniformLocation, unsigned int uniformValue);
	void updateUniform(const GLint uniformLocation, float uniformValue);
	void updateUniform(const GLint uniformLocation, vec2 uniformValue);
	void updateUniform(const GLint uniformLocation, vec4 uniformValue);
	void updateUniform(const GLint uniformLocation, const mat4& mat);
	void updateUniform(const GLint uniformLocation, const std::vector<vec4>& uniformValue);

	GLuint generateSSBO(GLuint SSBOSize, GLuint bufferBlockBindingPoint, const std::string& SSBOName);

	void updateSSBOImpl(const GLint& SSBO, size_t size, const void* SSBOValue);

	template<typename T>
	void updateSSBO(const GLint& SSBO, const T& SSBOValue)
	{
		updateSSBOImpl(SSBO, sizeof(T), &SSBOValue);
	}

	template<typename T>
	void updateSSBO(const GLint& SSBO, const std::vector<T>& SSBOValue)
	{
		updateSSBOImpl(SSBO, sizeof(T) * SSBOValue.size(), &SSBOValue[0]);
	}

	void bind2DDepthTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC);
	void bindCubemapDepthTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int textureIndex, unsigned int mipLevel);
	void bind2DColorTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex);
	void bind3DColorTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int layer);
	void bindCubemapTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel);
	void unbind2DColorTextureForWrite(GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex);
	void unbindCubemapTextureForWrite(GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel);
	void activateShaderProgram(GLShaderProgramComponent* GLShaderProgramComponent);

	void drawMesh(GLMeshDataComponent* GLMDC);
	void activateTexture(GLTextureDataComponent* GLTDC, int activateIndex);

	void activateRenderPass(GLRenderPassComponent * GLRPC);
	void cleanRenderBuffers(GLRenderPassComponent * GLRPC);
	void copyDepthBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest);
	void copyStencilBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest);
	void copyColorBuffer(GLRenderPassComponent* src, unsigned int srcIndex, GLRenderPassComponent* dest, unsigned int destIndex);

	vec4 readPixel(GLRenderPassComponent* GLRPC, unsigned int colorAttachmentIndex, GLint x, GLint y);
}

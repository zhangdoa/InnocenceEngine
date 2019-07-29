#pragma once
#include "../../Component/GLTextureDataComponent.h"
#include "../../Component/GLRenderPassDataComponent.h"

namespace GLHelper
{
	GLTextureDataDesc GetGLTextureDataDesc(TextureDataDesc textureDataDesc);
	GLenum GetTextureSamplerType(TextureSamplerType rhs);
	GLenum GetTextureWrapMethod(TextureWrapMethod rhs);
	GLenum GetTextureFilterParam(TextureFilterMethod rhs);
	GLenum GetTextureInternalFormat(TextureDataDesc textureDataDesc);
	GLenum GetTexturePixelDataFormat(TextureDataDesc textureDataDesc);
	GLenum GetTexturePixelDataType(TextureDataDesc textureDataDesc);
	GLsizei GetTexturePixelDataSize(TextureDataDesc textureDataDesc);

	bool GenerateDepthStencilState(DepthStencilDesc DSDesc, GLPipelineStateObject* PSO);
	bool GenerateBlendState(BlendDesc blendDesc, GLPipelineStateObject* PSO);
	bool GenerateRasterizerState(RasterizerDesc rasterizerDesc, GLPipelineStateObject* PSO);
	bool GenerateViewportState(ViewportDesc viewportDesc, GLPipelineStateObject* PSO);

	std::string LoadShaderFile(const std::string& path);
	bool AddShaderHandle(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const ShaderFilePath& shaderFilePath);
	bool ActivateTexture(GLTextureDataComponent * GLTDC, int activateIndex);

	bool AttachTextureToFramebuffer(GLTextureDataComponent * GLTDC, GLRenderPassDataComponent * GLRPC, unsigned int attachmentIndex, unsigned int textureIndex = 0, unsigned int mipLevel = 0, unsigned int layer = 0);
}
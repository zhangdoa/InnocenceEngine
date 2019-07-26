#pragma once
#include "../../Component/GLTextureDataComponent.h"
#include "../../Component/GLRenderPassDataComponent.h"

namespace GLHelper
{
	GLTextureDataDesc getGLTextureDataDesc(TextureDataDesc textureDataDesc);
	GLenum getTextureSamplerType(TextureSamplerType rhs);
	GLenum getTextureWrapMethod(TextureWrapMethod rhs);
	GLenum getTextureFilterParam(TextureFilterMethod rhs);
	GLenum getTextureInternalFormat(TextureDataDesc textureDataDesc);
	GLenum getTexturePixelDataFormat(TextureDataDesc textureDataDesc);
	GLenum getTexturePixelDataType(TexturePixelDataType rhs);
	GLsizei getTexturePixelDataSize(TextureDataDesc textureDataDesc);

	bool generateDepthStencilState(DepthStencilDesc DSDesc, GLPipelineStateObject* PSO);
	bool generateBlendState(BlendDesc blendDesc, GLPipelineStateObject* PSO);
	bool generateRasterizerState(RasterizerDesc rasterizerDesc, GLPipelineStateObject* PSO);
	bool generateViewportState(ViewportDesc viewportDesc, GLPipelineStateObject* PSO);

	std::string LoadShaderFile(const std::string& path);
	bool AddShaderHandle(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const ShaderFilePath& shaderFilePath);
}
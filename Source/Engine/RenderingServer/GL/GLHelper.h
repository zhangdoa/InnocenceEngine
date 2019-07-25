#pragma once
#include "../../Component/GLTextureDataComponent.h"

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

	std::string LoadShaderFile(const std::string& path);
	bool AddShaderHandle(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const ShaderFilePath& shaderFilePath);
}
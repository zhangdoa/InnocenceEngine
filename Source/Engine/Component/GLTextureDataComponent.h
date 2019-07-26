#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"
#include "TextureDataComponent.h"

struct GLTextureDataDesc
{
	GLenum textureSamplerType;
	GLenum textureWrapMethod;
	GLenum minFilterParam;
	GLenum magFilterParam;
	GLenum internalFormat;
	GLenum pixelDataFormat;
	GLenum pixelDataType;
	GLsizei pixelDataSize;
	GLsizei width;
	GLsizei height;
	GLsizei depth;
	float borderColor[4];
};

class GLTextureDataComponent : public TextureDataComponent
{
public:
	GLuint m_TO = 0;
	GLTextureDataDesc m_GLTextureDataDesc = GLTextureDataDesc();
};

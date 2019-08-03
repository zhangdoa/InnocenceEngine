#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"
#include "TextureDataComponent.h"

struct GLTextureDataDesc
{
	GLenum TextureSamplerType;
	GLenum TextureWrapMethod;
	GLenum MinFilterParam;
	GLenum MagFilterParam;
	GLenum InternalFormat;
	GLenum PixelDataFormat;
	GLenum PixelDataType;
	GLsizei PixelDataSize;
	GLsizei Width;
	GLsizei Height;
	GLsizei DepthOrArraySize;
	float BorderColor[4];
};

class GLTextureDataComponent : public TextureDataComponent
{
public:
	GLuint m_TO = 0;
	GLTextureDataDesc m_GLTextureDataDesc = GLTextureDataDesc();
};

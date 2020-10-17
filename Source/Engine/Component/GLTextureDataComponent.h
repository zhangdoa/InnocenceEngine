#pragma once
#include "../Common/InnoType.h"
#include "../RenderingServer/GL/GLHeaders.h"
#include "TextureDataComponent.h"

namespace Inno
{
	struct GLTextureDesc
	{
		GLenum TextureSampler;
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
		GLTextureDesc m_GLTextureDesc = GLTextureDesc();
	};
}

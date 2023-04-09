#pragma once
#include "GPUBufferComponent.h"
#include "../RenderingServer/GL/GLHeaders.h"

namespace Inno
{
	class GLGPUBufferComponent : public GPUBufferComponent
	{
	public:
		GLuint m_Handle = 0;
		GLenum m_BufferType = 0;
	};
}
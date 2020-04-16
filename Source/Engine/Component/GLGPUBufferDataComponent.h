#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingServer/GL/GLHeaders.h"

class GLGPUBufferDataComponent : public GPUBufferDataComponent
{
public:
	GLuint m_Handle = 0;
	GLenum m_BufferType = 0;
};
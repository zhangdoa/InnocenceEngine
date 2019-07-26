#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"

class GLGPUBufferDataComponent : public GPUBufferDataComponent
{
public:
	GLuint m_Handle = 0;
	GLenum m_BufferType = 0;
};
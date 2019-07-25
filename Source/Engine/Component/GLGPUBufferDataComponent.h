#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"

class GLGPUBufferDataComponent : public GPUBufferDataComponent
{
public:
	GLGPUBufferDataComponent() {};
	~GLGPUBufferDataComponent() {};

	GLuint m_Handle = 0;
};
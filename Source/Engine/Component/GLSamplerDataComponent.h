#pragma once
#include "SamplerDataComponent.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"

class GLSamplerDataComponent : public SamplerDataComponent
{
public:
	GLuint m_SO = 0;
};
#pragma once
#include "SamplerDataComponent.h"
#include "../RenderingServer/GL/GLHeaders.h"

namespace Inno
{
	class GLSamplerDataComponent : public SamplerDataComponent
	{
	public:
		GLuint m_SO = 0;
	};
}
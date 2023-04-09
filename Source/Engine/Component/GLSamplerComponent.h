#pragma once
#include "SamplerComponent.h"
#include "../RenderingServer/GL/GLHeaders.h"

namespace Inno
{
	class GLSamplerComponent : public SamplerComponent
	{
	public:
		GLuint m_SO = 0;
	};
}
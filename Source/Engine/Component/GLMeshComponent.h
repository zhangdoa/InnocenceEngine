#pragma once
#include "../RenderingServer/GL/GLHeaders.h"
#include "MeshComponent.h"

namespace Inno
{
	class GLMeshComponent : public MeshComponent
	{
	public:
		GLuint m_VAO = 0;
		GLuint m_VBO = 0;
		GLuint m_IBO = 0;
	};
}

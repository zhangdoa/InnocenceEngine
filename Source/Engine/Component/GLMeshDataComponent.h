#pragma once
#include "../Common/InnoType.h"
#include "../RenderingServer/GL/GLHeaders.h"
#include "MeshDataComponent.h"

class GLMeshDataComponent : public MeshDataComponent
{
public:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

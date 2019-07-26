#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/GLRenderingBackend/GLHeaders.h"
#include "MeshDataComponent.h"

class GLMeshDataComponent : public MeshDataComponent
{
public:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

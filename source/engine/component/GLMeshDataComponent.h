#pragma once
#include "../common/InnoType.h"
#include "../system/GLRenderingBackend/GLHeaders.h"
#include "MeshDataComponent.h"

class GLMeshDataComponent : public MeshDataComponent
{
public:
	GLMeshDataComponent() {};
	~GLMeshDataComponent() {};

	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

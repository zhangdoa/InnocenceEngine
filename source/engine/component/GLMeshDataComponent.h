#pragma once
#include "MeshDataComponent.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLMeshDataComponent : public BaseComponent
{
public:
	GLMeshDataComponent() {};
	~GLMeshDataComponent() {};

	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};


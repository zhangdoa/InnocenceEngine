#pragma once
#include "MeshDataComponent.h"
#include "common/GLHeaders.h"

class GLMeshDataComponent : public MeshDataComponent
{
public:
	GLMeshDataComponent() {};
	~GLMeshDataComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;

	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};


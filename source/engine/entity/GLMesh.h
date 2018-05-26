#pragma once
#include "common/GLHeaders.h"
#include "BaseMesh.h"

class GLMesh : public BaseMesh
{
public:
	GLMesh() {};
	~GLMesh() {};

	void initialize() override;
	void draw() override;
	void shutdown() override;

private:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};
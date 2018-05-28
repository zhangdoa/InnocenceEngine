#pragma once
#include "BaseComponent.h"
#include "entity/InnoMath.h"
#include "common/GLHeaders.h"

class GLMeshDataComponent : public BaseComponent
{
public:
	GLMeshDataComponent() {};
	~GLMeshDataComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;

	meshID m_meshID;
	meshType m_meshType;

	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};


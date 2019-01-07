#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

class GLMeshDataComponent
{
public:
	GLMeshDataComponent() {};
	~GLMeshDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};


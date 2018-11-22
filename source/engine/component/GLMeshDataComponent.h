#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

class GLMeshDataComponent
{
public:
	GLMeshDataComponent() {};
	~GLMeshDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};


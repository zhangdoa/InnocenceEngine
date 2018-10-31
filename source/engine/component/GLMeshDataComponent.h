#pragma once
#include "../common/InnoType.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLMeshDataComponent
{
public:
	GLMeshDataComponent() {};
	~GLMeshDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};


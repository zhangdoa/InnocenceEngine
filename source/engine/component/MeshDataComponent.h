#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

class MeshDataComponent
{
public:
	MeshDataComponent() {};
	~MeshDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	meshType m_meshType = meshType::NORMAL;
	Vertex* m_vertices;
	size_t m_verticesSize;
	Index* m_indices;
	size_t m_indicesSize;

	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
};


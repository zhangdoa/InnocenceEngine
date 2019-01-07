#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

class MeshDataComponent
{
public:
	MeshDataComponent() {};
	~MeshDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	MeshUsageType m_meshUsageType = MeshUsageType::NORMAL;
	MeshPrimitiveTopology m_meshDrawMethod = MeshPrimitiveTopology::TRIANGLE;
	size_t m_indicesSize = 0;
	std::vector<Vertex> m_vertices;
	std::vector<Index> m_indices;
};


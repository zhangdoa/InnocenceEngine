#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

class MeshDataComponent
{
public:
	MeshDataComponent() {};
	~MeshDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	MeshUsageType m_meshUsageType = MeshUsageType::NORMAL;
	std::vector<Vertex> m_vertices;
	std::vector<Index> m_indices;
	size_t m_indicesSize = 0;
	MeshPrimitiveTopology m_meshDrawMethod = MeshPrimitiveTopology::TRIANGLE;
};


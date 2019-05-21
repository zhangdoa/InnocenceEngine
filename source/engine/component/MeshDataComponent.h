#pragma once
#include "../common/InnoComponent.h"
#include "../common/InnoMath.h"
#include "PhysicsDataComponent.h"

class MeshDataComponent : public InnoComponent
{
public:
	MeshDataComponent() {};
	~MeshDataComponent() {};

	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	MeshShapeType m_meshShapeType = MeshShapeType::LINE;
	size_t m_indicesSize = 0;
	PhysicsDataComponent* m_PDC;
	std::vector<Vertex> m_vertices;
	std::vector<Index> m_indices;
};

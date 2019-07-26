#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"
#include "PhysicsDataComponent.h"
#include "SkeletonDataComponent.h"

class MeshDataComponent : public InnoComponent
{
public:
	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	MeshShapeType m_meshShapeType = MeshShapeType::LINE;
	size_t m_indicesSize = 0;
	PhysicsDataComponent* m_PDC = 0;
	SkeletonDataComponent* m_SDC = 0;
	InnoArray<Vertex> m_vertices;
	InnoArray<Index> m_indices;
};

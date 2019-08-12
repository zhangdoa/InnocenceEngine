#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoComponent.h"
#include "SkeletonDataComponent.h"

class PhysicsDataComponent;
class MeshDataComponent : public InnoComponent
{
public:
	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	MeshShapeType m_meshShapeType = MeshShapeType::Line;
	size_t m_indicesSize = 0;
	PhysicsDataComponent* m_PDC = 0;
	SkeletonDataComponent* m_SDC = 0;
	Array<Vertex> m_vertices;
	Array<Index> m_indices;
};

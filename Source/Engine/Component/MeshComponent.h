#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoObject.h"
#include "SkeletonComponent.h"

namespace Inno
{
	class MeshComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 6; };
		static const char* GetTypeName() { return "MeshComponent"; };

		MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
		MeshSource m_meshSource = MeshSource::Procedural;
		ProceduralMeshShape m_proceduralMeshShape = ProceduralMeshShape::Triangle;
		size_t m_indicesSize = 0;
		SkeletonComponent* m_SamplerComp = 0;
		Array<Vertex> m_vertices;
		Array<Index> m_indices;
	};
}

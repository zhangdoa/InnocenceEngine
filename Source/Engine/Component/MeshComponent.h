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

		MeshPrimitiveTopology m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
		MeshSource m_MeshSource = MeshSource::Procedural;
		ProceduralMeshShape m_ProceduralMeshShape = ProceduralMeshShape::Triangle;
		size_t m_IndexCount = 0;
		SkeletonComponent* m_SkeletonComp = 0;

		Array<Vertex> m_Vertices;
		Array<Index> m_Indices;
	};
}

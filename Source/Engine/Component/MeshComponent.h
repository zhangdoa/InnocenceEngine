#pragma once
#include "../Common/Object.h"
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Array.h"
#include "../Common/MathHelper.h"

namespace Inno
{
	class MeshComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 6; };
		static const char* GetTypeName() { return "MeshComponent"; };

		MeshShape m_MeshShape = MeshShape::Customized;
		size_t m_IndexCount = 0;

		Array<Vertex> m_Vertices;
		Array<Index> m_Indices;
	};
}

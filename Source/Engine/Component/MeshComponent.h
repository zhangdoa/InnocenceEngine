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
		void* m_MappedMemory_VB = nullptr;
		void* m_MappedMemory_IB = nullptr;
		size_t m_IndexCount = 0;
		bool m_NeedUploadToGPU = false;

		Array<Vertex> m_Vertices;
		Array<Index> m_Indices;
	};
}

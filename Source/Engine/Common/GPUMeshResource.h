#pragma once
#include "GraphicsPrimitive.h"
#include "Object.h"
#include "MathHelper.h"

namespace Inno
{
	struct GPUMeshResourceHandle
	{
		uint32_t m_Index = UINT32_MAX;
		bool IsValid() const { return m_Index != UINT32_MAX; }
	};

	inline constexpr GPUMeshResourceHandle INVALID_GPU_MESH_HANDLE = {};

	struct GPUMeshResource
	{
		ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
		ObjectStatus m_Status = ObjectStatus::Invalid;
		ObjectName m_Name = "";

		GPUBufferView m_VertexBufferView;
		GPUBufferView m_IndexBufferView;

		void* m_MappedMemory_VB = nullptr;
		void* m_MappedMemory_IB = nullptr;

		AABB m_AABB;

		uint32_t GetIndexCount() const
		{
			if (m_IndexBufferView.m_StrideInBytes == 0)
				return 0;
			return m_IndexBufferView.m_SizeInBytes / m_IndexBufferView.m_StrideInBytes;
		}
	};
}

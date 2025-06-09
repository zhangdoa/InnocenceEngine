#pragma once
#include "../Common/Object.h"
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Array.h"
#include "../Common/MathHelper.h"

namespace Inno
{
	// API-agnostic GPU buffer view handles
	// These map to D3D12_VERTEX_BUFFER_VIEW/D3D12_INDEX_BUFFER_VIEW in DX12
	// and VkBuffer with offsets in Vulkan
	struct GPUBufferView
	{
		uint64_t bufferHandle = 0;        // GPU buffer resource handle
		uint64_t bufferLocation = 0;      // GPU virtual address (for DX12) or offset (for VK)
		uint32_t sizeInBytes = 0;         // Size of the view in bytes
		uint32_t strideInBytes = 0;       // Stride for vertex buffer, unused for index buffer

		bool IsValid() const
		{
			return bufferHandle != 0 && sizeInBytes > 0 && strideInBytes > 0;
		}
	};

	class MeshComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 6; };
		static const char* GetTypeName() { return "MeshComponent"; };

		uint64_t m_CollisionComponent = 0;
		void* m_MappedMemory_VB = nullptr;
		void* m_MappedMemory_IB = nullptr;
		GPUBufferView m_VertexBufferView;
		GPUBufferView m_IndexBufferView;
	};
}
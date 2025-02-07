#pragma once
#include "GPUResourceComponent.h"

INNO_ENUM(GPUBufferUsage, None, IndirectDraw, IndirectDispatch, AtomicCounter);

namespace Inno
{
	class GPUBufferComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 14; };
		static const char* GetTypeName() { return "GPUBufferComponent"; };

		size_t m_ElementCount = 0;
		size_t m_ElementSize = 0;
		size_t m_TotalSize = 0;
		GPUBufferUsage m_Usage = GPUBufferUsage::None;
		void* m_InitialData = 0;
		IIndirectDrawCommandList* m_IndirectDrawCommandList = 0;
		std::vector<IMappedMemory*> m_MappedMemories;
		std::vector<IDeviceMemory*> m_DeviceMemories;
	};
}

#pragma once
#include "GPUResourceComponent.h"

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
		bool m_isAtomicCounter = false;
		void* m_InitialData = 0;
		std::vector<IMappedMemory*> m_MappedMemories;
		std::vector<IDeviceMemory*> m_DeviceMemories;
	};
}

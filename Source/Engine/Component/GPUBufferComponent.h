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
		GPUBufferUsage m_Usage = GPUBufferUsage::None;
		void* m_InitialData = 0;
		std::vector<IMappedMemory*> m_MappedMemories;
		std::vector<IDeviceMemory*> m_DeviceMemories;

		mutable std::vector<uint32_t> m_CurrentState;
		
		uint32_t GetCurrentState(uint32_t frameIndex) const
		{
			if (frameIndex < m_CurrentState.size())
				return m_CurrentState[frameIndex];

			return 0;
		}
		
		void SetCurrentState(uint32_t frameIndex, uint32_t state) const
		{
			if (frameIndex < m_CurrentState.size())
				m_CurrentState[frameIndex] = state;
		}
	};
}

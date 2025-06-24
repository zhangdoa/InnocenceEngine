#pragma once
#include "GPUResourceComponent.h"

namespace Inno
{
	class TextureComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 8; };
		static const char* GetTypeName() { return "TextureComponent"; };
		TextureDesc m_TextureDesc = {};

		std::vector<void*> m_GPUResources;
		
		// Track current state per frame buffer (dynamic array like m_GPUResources)
		mutable std::vector<uint32_t> m_CurrentState;
		
		inline uint32_t GetHandleIndex(uint32_t frameIndex, uint32_t mipLevel) const
		{
			return frameIndex * m_TextureDesc.MipLevels + mipLevel;
		}
		
		void* GetGPUResource(uint32_t frameIndex) const
		{
			if (!m_TextureDesc.IsMultiBuffer)
				return m_GPUResources[0];
		
			if (frameIndex < m_GPUResources.size())
				return m_GPUResources[frameIndex];
			return nullptr;
		}
		
		uint32_t GetCurrentState(uint32_t frameIndex) const
		{
			if (!m_TextureDesc.IsMultiBuffer)
				return m_CurrentState[0];
		
			if (frameIndex < m_CurrentState.size())
				return m_CurrentState[frameIndex];

			return 0;
		}
		
		void SetCurrentState(uint32_t frameIndex, uint32_t state) const
		{
			if (!m_TextureDesc.IsMultiBuffer)
				m_CurrentState[0] = state;
		
			if (frameIndex < m_CurrentState.size())
				m_CurrentState[frameIndex] = state;
		}
	};
}

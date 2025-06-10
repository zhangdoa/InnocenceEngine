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
		uint32_t m_ReadState = 0;
		uint32_t m_WriteState = 0;
		uint32_t m_CurrentState = 0;

		std::vector<void*> m_GPUResources;
		
		inline uint32_t GetHandleIndex(uint32_t frameIndex, uint32_t mipLevel) const
		{
			return frameIndex * m_TextureDesc.MipLevels + mipLevel;
		}
		
		void* GetGPUResource(uint32_t frameIndex) const
		{
			if (frameIndex < m_GPUResources.size())
				return m_GPUResources[frameIndex];
			return nullptr;
		}
	};
}

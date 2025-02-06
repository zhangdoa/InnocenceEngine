#pragma once
#include "GPUResourceComponent.h"

namespace Inno
{
	// @TODO: Implement multi-buffering for textures.
	class TextureComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 8; };
		static const char* GetTypeName() { return "TextureComponent"; };
		TextureDesc m_TextureDesc = {};
	
		std::vector<IMappedMemory*> m_MappedMemories;
		std::vector<IDeviceMemory*> m_DeviceMemories;		
		void* m_InitialData = 0;
		uint32_t m_PixelDataSize = 0;
	};
}

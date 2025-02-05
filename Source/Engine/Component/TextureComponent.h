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
	
		void* m_TextureData = 0;
		bool m_NeedUploadToGPU = false;
	};
}

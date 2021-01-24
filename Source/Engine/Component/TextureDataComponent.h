#pragma once
#include "GPUResourceComponent.h"

namespace Inno
{
	class TextureDataComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 8; };
		static const char* GetTypeName() { return "TextureDataComponent"; };

		TextureDesc m_TextureDesc = {};
		void* m_TextureData = 0;
	};
}

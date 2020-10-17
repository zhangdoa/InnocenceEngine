#pragma once
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoObject.h"

namespace Inno
{
	class TextureDataComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 8; };
		static char* GetTypeName() { return "TextureDataComponent"; };

		TextureDesc m_TextureDesc = {};
		void* m_TextureData = 0;
		IResourceBinder* m_ResourceBinder = 0;
	};
}

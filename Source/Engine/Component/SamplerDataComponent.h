#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoObject.h"

namespace Inno
{
	struct SamplerDesc
	{
		TextureFilterMethod m_MinFilterMethod = TextureFilterMethod::Linear;
		TextureFilterMethod m_MagFilterMethod = TextureFilterMethod::Linear;
		TextureWrapMethod m_WrapMethodU = TextureWrapMethod::Edge;
		TextureWrapMethod m_WrapMethodV = TextureWrapMethod::Edge;
		TextureWrapMethod m_WrapMethodW = TextureWrapMethod::Edge;
		bool m_UseMipMap = false;
		float m_BorderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float m_MinLOD = 0.0f;
		float m_MaxLOD = 3.402823466e+38f;
		uint32_t m_MaxAnisotropy = 1;
	};

	class SamplerDataComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 13; };
		static char* GetTypeName() { return "SamplerDataComponent"; };

		SamplerDesc m_SamplerDesc = {};
		IResourceBinder* m_ResourceBinder = 0;
	};
}
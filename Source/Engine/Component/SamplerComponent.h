#pragma once
#include "GPUResourceComponent.h"

namespace Inno
{
	struct SamplerDesc
	{
		TextureFilterMethod m_MinFilterMethod = TextureFilterMethod::Linear;
		TextureFilterMethod m_MagFilterMethod = TextureFilterMethod::Linear;
		TextureWrapMethod m_WrapMethodU = TextureWrapMethod::Edge;
		TextureWrapMethod m_WrapMethodV = TextureWrapMethod::Edge;
		TextureWrapMethod m_WrapMethodW = TextureWrapMethod::Edge;
		float m_BorderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float m_MinLOD = 0.0f;
		float m_MaxLOD = 3.402823466e+38f;
		uint32_t m_MaxAnisotropy = 1;
	};

	class SamplerComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 13; };
		static const char* GetTypeName() { return "SamplerComponent"; };

		SamplerDesc m_SamplerDesc = {};
	};
}
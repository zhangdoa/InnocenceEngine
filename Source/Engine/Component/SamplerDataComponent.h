#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoComponent.h"

struct SamplerDesc
{
	TextureFilterMethod m_MinFilterMethod = TextureFilterMethod::Mip;
	TextureFilterMethod m_MagFilterMethod = TextureFilterMethod::Linear;
	TextureWrapMethod m_WrapMethodU = TextureWrapMethod::Edge;
	TextureWrapMethod m_WrapMethodV = TextureWrapMethod::Edge;
	TextureWrapMethod m_WrapMethodW = TextureWrapMethod::Edge;
	float m_BorderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_MinLOD = 0.0f;
	float m_MaxLOD = 3.402823466e+38f;
	unsigned int m_MaxAnisotropy = 1;
};

class SamplerDataComponent : public InnoComponent
{
public:
	SamplerDesc m_SamplerDesc = {};
	IResourceBinder* m_ResourceBinder = 0;
};

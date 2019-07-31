#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoComponent.h"

struct SamplerDesc
{
	TextureFilterMethod m_MinFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
	TextureFilterMethod m_MagFilterMethod = TextureFilterMethod::LINEAR;
	TextureWrapMethod m_WrapMethodU = TextureWrapMethod::CLAMP_TO_EDGE;
	TextureWrapMethod m_WrapMethodV = TextureWrapMethod::CLAMP_TO_EDGE;
	TextureWrapMethod m_WrapMethodW = TextureWrapMethod::CLAMP_TO_EDGE;
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

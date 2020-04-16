#pragma once
#include "SamplerDataComponent.h"
#include "../RenderingServer/DX11/DX11Headers.h"

class DX11SamplerDataComponent : public SamplerDataComponent
{
public:
	D3D11_SAMPLER_DESC m_DX11SamplerDesc = {};
	ID3D11SamplerState* m_SamplerState = 0;
};
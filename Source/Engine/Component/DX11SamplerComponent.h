#pragma once
#include "SamplerComponent.h"
#include "../RenderingServer/DX11/DX11Headers.h"

namespace Inno
{
	class DX11SamplerComponent : public SamplerComponent
	{
	public:
		D3D11_SAMPLER_DESC m_DX11SamplerDesc = {};
		ID3D11SamplerState* m_SamplerState = 0;
	};
}
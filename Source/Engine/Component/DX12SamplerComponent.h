#pragma once
#include "SamplerComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{
	class DX12SamplerComponent : public SamplerComponent
	{
	public:
		DX12Sampler m_Sampler = {};
	};
}
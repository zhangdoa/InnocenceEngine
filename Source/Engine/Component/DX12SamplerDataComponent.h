#pragma once
#include "SamplerDataComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{
	class DX12SamplerDataComponent : public SamplerDataComponent
	{
	public:
		DX12Sampler m_Sampler = {};
	};
}
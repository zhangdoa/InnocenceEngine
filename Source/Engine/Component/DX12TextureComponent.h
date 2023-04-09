#pragma once
#include "../Common/InnoType.h"
#include "../RenderingServer/DX12/DX12Headers.h"
#include "TextureComponent.h"

namespace Inno
{
	class DX12TextureComponent : public TextureComponent
	{
	public:
		ComPtr<ID3D12Resource> m_ResourceHandle = 0;
		D3D12_RESOURCE_DESC m_DX12TextureDesc = {};
		uint32_t m_PixelDataSize = 0;
		DX12SRV m_SRV = {};
		DX12UAV m_UAV = {};
		D3D12_RESOURCE_STATES m_WriteState;
		D3D12_RESOURCE_STATES m_ReadState;
		D3D12_RESOURCE_STATES m_CurrentState;
	};
}
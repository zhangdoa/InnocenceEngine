#pragma once
#include "../RenderingServer/DX12/DX12Headers.h"
#include "TextureComponent.h"

namespace Inno
{
	class DX12TextureComponent : public TextureComponent
	{
	public:
		D3D12_RESOURCE_DESC m_DX12TextureDesc = {};

		D3D12_RESOURCE_STATES m_WriteState;
		D3D12_RESOURCE_STATES m_ReadState;
		D3D12_RESOURCE_STATES m_CurrentState;
	};
}
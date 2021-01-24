#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingServer/DX11/DX11Headers.h"

namespace Inno
{
	class DX11GPUBufferDataComponent : public GPUBufferDataComponent
	{
	public:
		D3D11_BUFFER_DESC m_BufferDesc = {};
		ID3D11Buffer* m_Buffer = 0;
		ID3D11ShaderResourceView* m_SRV = 0;
		ID3D11UnorderedAccessView* m_UAV = 0;
		bool isAtomicCounter = false;
	};
}
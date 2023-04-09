#pragma once
#include "../Common/InnoType.h"
#include "../RenderingServer/DX12/DX12Headers.h"
#include "MeshComponent.h"

namespace Inno
{
	class DX12MeshComponent : public MeshComponent
	{
	public:
		ComPtr<ID3D12Resource> m_vertexBuffer = 0;
		ComPtr<ID3D12Resource> m_indexBuffer = 0;
		D3D12_VERTEX_BUFFER_VIEW m_VBV;
		D3D12_INDEX_BUFFER_VIEW m_IBV;
	};
}

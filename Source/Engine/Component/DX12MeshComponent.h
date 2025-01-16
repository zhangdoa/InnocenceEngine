#pragma once
#include "../RenderingServer/DX12/DX12Headers.h"
#include "MeshComponent.h"

namespace Inno
{
	class DX12MeshComponent : public MeshComponent
	{
	public:
		ComPtr<ID3D12Resource> m_UploadHeapBuffer_VB = 0;
		ComPtr<ID3D12Resource> m_UploadHeapBuffer_IB = 0;
		ComPtr<ID3D12Resource> m_DefaultHeapBuffer_VB = 0;
		ComPtr<ID3D12Resource> m_DefaultHeapBuffer_IB = 0;
		D3D12_VERTEX_BUFFER_VIEW m_VBV;
		D3D12_INDEX_BUFFER_VIEW m_IBV;
		void* m_MappedUploadHeapBuffer_VB;
		void* m_MappedUploadHeapBuffer_IB;
	};
}

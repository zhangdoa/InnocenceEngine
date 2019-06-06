#pragma once
#include "../Common/InnoType.h"
#include "../System/RenderingBackend/DX12RenderingBackend/DX12Headers.h"
#include "MeshDataComponent.h"

class DX12MeshDataComponent : public MeshDataComponent
{
public:
	DX12MeshDataComponent() {};
	~DX12MeshDataComponent() {};

	ID3D12Resource* m_vertexBuffer = 0;
	ID3D12Resource* m_indexBuffer = 0;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};

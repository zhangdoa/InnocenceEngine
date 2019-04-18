#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DX12Headers.h"
#include "MeshDataComponent.h"

class DX12MeshDataComponent : public MeshDataComponent
{
public:
	DX12MeshDataComponent() {};
	~DX12MeshDataComponent() {};

	ID3D12Resource* m_vertexBuffer = 0;
	ID3D12Resource* m_indexBuffer = 0;
};

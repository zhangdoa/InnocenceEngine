#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DXHeaders.h"

class DX12MeshDataComponent
{
public:
	DX12MeshDataComponent() {};
	~DX12MeshDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D11Buffer* m_vertexBuffer = 0;
	ID3D11Buffer* m_indexBuffer = 0;
};

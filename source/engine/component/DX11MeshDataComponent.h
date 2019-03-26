#pragma once
#include "../common/InnoType.h"
#include "../system/DX11RenderingBackend/DXHeaders.h"

class DX11MeshDataComponent
{
public:
	DX11MeshDataComponent() {};
	~DX11MeshDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D11Buffer* m_vertexBuffer = 0;
	ID3D11Buffer* m_indexBuffer = 0;
};

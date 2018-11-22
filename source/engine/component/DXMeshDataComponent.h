#pragma once
#include "../common/InnoType.h"
#include "../system/DXHeaders.h"

class DXMeshDataComponent
{
public:
	DXMeshDataComponent() {};
	~DXMeshDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	ID3D11Buffer* m_vertexBuffer = 0;
	ID3D11Buffer* m_indexBuffer = 0;
};


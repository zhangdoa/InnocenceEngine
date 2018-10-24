#pragma once
#include "MeshDataComponent.h"
#include "../system/HighLevelSystem/DXHeaders.h"

class DXMeshDataComponent : public BaseComponent
{
public:
	DXMeshDataComponent() {};
	~DXMeshDataComponent() {};

	ID3D11Buffer* m_vertexBuffer = 0;
	ID3D11Buffer* m_indexBuffer = 0;
};


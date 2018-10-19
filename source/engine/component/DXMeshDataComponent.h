#pragma once
#include "MeshDataComponent.h"
#include "../system/HighLevelSystem/DXHeaders.h"

class DXMeshDataComponent : public MeshDataComponent
{
public:
	DXMeshDataComponent() {};
	~DXMeshDataComponent() {};

	ID3D11Buffer* m_vertexBuffer = 0;
	ID3D11Buffer* m_indexBuffer = 0;
};


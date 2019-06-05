#pragma once
#include "../Common/InnoType.h"
#include "../System/RenderingBackend/DX11RenderingBackend/DX11Headers.h"
#include "MeshDataComponent.h"

class DX11MeshDataComponent : public MeshDataComponent
{
public:
	DX11MeshDataComponent() {};
	~DX11MeshDataComponent() {};

	ID3D11Buffer* m_vertexBuffer = 0;
	ID3D11Buffer* m_indexBuffer = 0;
};

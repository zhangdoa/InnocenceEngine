#pragma once
#include "../Common/InnoType.h"
#include "../RenderingServer/DX11/DX11Headers.h"
#include "MeshDataComponent.h"

namespace Inno
{
	class DX11MeshDataComponent : public MeshDataComponent
	{
	public:
		ID3D11Buffer* m_vertexBuffer = 0;
		ID3D11Buffer* m_indexBuffer = 0;
	};
}

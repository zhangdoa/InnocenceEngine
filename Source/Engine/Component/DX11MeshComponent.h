#pragma once
#include "../RenderingServer/DX11/DX11Headers.h"
#include "MeshComponent.h"

namespace Inno
{
	class DX11MeshComponent : public MeshComponent
	{
	public:
		ID3D11Buffer* m_vertexBuffer = 0;
		ID3D11Buffer* m_indexBuffer = 0;
	};
}

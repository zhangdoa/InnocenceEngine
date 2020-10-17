#pragma once
#include "ShaderProgramComponent.h"
#include "../RenderingServer/DX11/DX11Headers.h"

namespace Inno
{
	class DX11ShaderProgramComponent : public ShaderProgramComponent
	{
	public:
		ID3D11VertexShader* m_VSHandle = 0;
		ID3D11HullShader* m_HSHandle = 0;
		ID3D11DomainShader* m_DSHandle = 0;
		ID3D11GeometryShader* m_GSHandle = 0;
		ID3D11PixelShader* m_FSHandle = 0;
		ID3D11ComputeShader* m_CSHandle = 0;
	};
}
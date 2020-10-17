#pragma once
#include "ShaderProgramComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{
	class DX12ShaderProgramComponent : public ShaderProgramComponent
	{
	public:
		ComPtr<ID3DBlob> m_VSBuffer = 0;
		ComPtr<ID3DBlob> m_HSBuffer = 0;
		ComPtr<ID3DBlob> m_DSBuffer = 0;
		ComPtr<ID3DBlob> m_GSBuffer = 0;
		ComPtr<ID3DBlob> m_PSBuffer = 0;
		ComPtr<ID3DBlob> m_CSBuffer = 0;
	};
}
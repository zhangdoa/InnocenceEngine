#pragma once
#include "ShaderProgramComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{
	class DX12ShaderProgramComponent : public ShaderProgramComponent
	{
	public:
#ifdef USE_DXIL
		std::vector<char> m_VSBuffer;
		std::vector<char> m_HSBuffer;
		std::vector<char> m_DSBuffer;
		std::vector<char> m_GSBuffer;
		std::vector<char> m_PSBuffer;
		std::vector<char> m_CSBuffer;
		std::vector<char> m_RayGenBuffer;
		std::vector<char> m_AnyHitBuffer;
		std::vector<char> m_ClosestHitBuffer;
		std::vector<char> m_MissBuffer;
#else
		ComPtr<ID3DBlob> m_VSBuffer = 0;
		ComPtr<ID3DBlob> m_HSBuffer = 0;
		ComPtr<ID3DBlob> m_DSBuffer = 0;
		ComPtr<ID3DBlob> m_GSBuffer = 0;
		ComPtr<ID3DBlob> m_PSBuffer = 0;
		ComPtr<ID3DBlob> m_CSBuffer = 0;
		ComPtr<ID3DBlob> m_RayGenBuffer = 0;
		ComPtr<ID3DBlob> m_AnyHitBuffer = 0;
		ComPtr<ID3DBlob> m_ClosestHitBuffer = 0;
		ComPtr<ID3DBlob> m_MissBuffer = 0;
#endif
	};
} // namespace Inno
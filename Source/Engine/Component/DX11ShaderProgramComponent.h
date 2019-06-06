#pragma once
#include "../Common/InnoType.h"
#include "../System/RenderingBackend/DX11RenderingBackend/DX11Headers.h"

class DX11ShaderProgramComponent
{
public:
	DX11ShaderProgramComponent() {};
	~DX11ShaderProgramComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	ID3D11VertexShader* m_vertexShader = 0;
	ID3D11InputLayout* m_inputLayout = 0;
	ID3D11PixelShader* m_pixelShader = 0;
	D3D11_SAMPLER_DESC m_samplerDesc = D3D11_SAMPLER_DESC();
	ID3D11SamplerState* m_samplerState = 0;
	ID3D11ComputeShader* m_computeShader = 0;
};
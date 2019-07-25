#pragma once
#include "ShaderProgramComponent.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"

class DX12ShaderProgramComponent : public ShaderProgramComponent
{
public:
	DX12ShaderProgramComponent() {};
	~DX12ShaderProgramComponent() {};

	ID3DBlob* m_vertexShader = 0;
	ID3DBlob* m_pixelShader = 0;
	D3D12_SAMPLER_DESC m_samplerDesc = D3D12_SAMPLER_DESC();
	D3D12_CPU_DESCRIPTOR_HANDLE m_samplerCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_samplerGPUHandle;
};
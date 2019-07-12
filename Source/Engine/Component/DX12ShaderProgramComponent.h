#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"

class DX12ShaderProgramComponent
{
public:
	DX12ShaderProgramComponent() {};
	~DX12ShaderProgramComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	ID3DBlob* m_vertexShader = 0;
	ID3DBlob* m_pixelShader = 0;
	D3D12_SAMPLER_DESC m_samplerDesc = D3D12_SAMPLER_DESC();
	D3D12_CPU_DESCRIPTOR_HANDLE m_samplerCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_samplerGPUHandle;
};
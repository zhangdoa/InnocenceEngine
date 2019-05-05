#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DX12Headers.h"

class DX12ShaderProgramComponent
{
public:
	DX12ShaderProgramComponent() {};
	~DX12ShaderProgramComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3DBlob* m_vertexShader = 0;
	ID3DBlob* m_pixelShader = 0;
	D3D12_SAMPLER_DESC m_samplerDesc = D3D12_SAMPLER_DESC();
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
};
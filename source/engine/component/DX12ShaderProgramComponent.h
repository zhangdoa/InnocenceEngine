#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DX12Headers.h"

struct DX12CBuffer
{
	D3D12_RESOURCE_DESC m_CBufferDesc = D3D12_RESOURCE_DESC();
	ID3D12Resource* m_CBufferPtr = 0;
};

class DX12ShaderProgramComponent
{
public:
	DX12ShaderProgramComponent() {};
	~DX12ShaderProgramComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3DBlob* m_vertexShader = 0;
	std::vector<DX12CBuffer> m_VSCBuffers;
	ID3DBlob* m_pixelShader = 0;
	D3D12_SAMPLER_DESC m_samplerDesc = D3D12_SAMPLER_DESC();
	std::vector<DX12CBuffer> m_PSCBuffers;
};
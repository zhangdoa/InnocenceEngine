#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"
#include "TextureDataComponent.h"

class DX12TextureDataComponent : public TextureDataComponent
{
public:
	DX12TextureDataComponent() {};
	~DX12TextureDataComponent() {};

	ID3D12Resource* m_texture = 0;
	D3D12_RESOURCE_DESC m_DX12TextureDataDesc = {};
	ID3D12Resource* m_SRV = 0;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
};

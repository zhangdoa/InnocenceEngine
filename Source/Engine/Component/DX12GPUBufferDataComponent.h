#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

class DX12GPUBufferDataComponent : public GPUBufferDataComponent
{
public:
	ComPtr<ID3D12Resource> m_DefaultHeapResourceHandle = 0;
	ComPtr<ID3D12Resource> m_UploadHeapResourceHandle = 0;
	void* m_MappedMemory = 0;
};
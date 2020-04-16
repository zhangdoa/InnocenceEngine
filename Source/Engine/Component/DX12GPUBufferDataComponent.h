#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

class DX12GPUBufferDataComponent : public GPUBufferDataComponent
{
public:
	ID3D12Resource* m_DefaultHeapResourceHandle = 0;
	ID3D12Resource* m_UploadHeapResourceHandle = 0;
	void* m_MappedMemory = 0;
};
#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"

class DX12GPUBufferDataComponent : public GPUBufferDataComponent
{
public:
	ID3D12Resource* m_ResourceHandle = 0;
	void* m_MappedMemory = 0;
	IResourceBinder* m_ResourceBinder = 0;
};
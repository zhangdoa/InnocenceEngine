#pragma once
#include "GPUBufferComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{	
	struct DX12MappedMemory : public IMappedMemory
	{
		ComPtr<ID3D12Resource> m_UploadHeapBuffer = 0;
		DX12CBV m_CBV = {};
	};
	
	struct DX12DeviceMemory
	{
		ComPtr<ID3D12Resource> m_DefaultHeapBuffer = 0;		
		DX12SRV m_SRV = {};
		DX12UAV m_UAV = {};
	};

	class DX12GPUBufferComponent : public GPUBufferComponent
	{
	public:
		std::vector<DX12DeviceMemory*> m_DeviceMemories;
	};
}
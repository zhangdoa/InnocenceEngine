#pragma once
#include "GPUBufferDataComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{
	class DX12GPUBufferDataComponent : public GPUBufferDataComponent
	{
	public:
		ComPtr<ID3D12Resource> m_DefaultHeapResourceHandle = 0;
		ComPtr<ID3D12Resource> m_UploadHeapResourceHandle = 0;
		DX12SRV m_SRV = {};
		DX12UAV m_UAV = {};
		void* m_MappedMemory = 0;
	};
}
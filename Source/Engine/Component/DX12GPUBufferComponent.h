#pragma once
#include "GPUBufferComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{
	class DX12GPUBufferComponent : public GPUBufferComponent
	{
	public:
		ComPtr<ID3D12Resource> m_DefaultHeapBuffer = 0;
		ComPtr<ID3D12Resource> m_UploadHeapBuffer = 0;
		DX12SRV m_SRV = {};
		DX12UAV m_UAV = {};
		void* m_MappedMemory = 0;
	};
}
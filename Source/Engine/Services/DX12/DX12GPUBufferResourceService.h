#pragma once
#include "../GPUBufferResourceService.h"
#include "DX12Headers.h"
#include "DX12Context.h"

namespace Inno
{
	class DX12GPUBufferResourceService : public GPUBufferResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12GPUBufferResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		bool Delete(GPUBufferComponent* ptr) override;
		bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) override;
		bool Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) override;

		bool CreateRaytracingResources();
		bool ReleaseRaytracingResources();

	protected:
		bool InitializeImpl(GPUBufferComponent* gpuBuffer) override;
		bool InitializeImpl(EntityID entity) override;
		bool OnSceneLoadingStart() override;

	private:
		bool CreateSRV(GPUBufferComponent* gpuBuffer);
		bool CreateUAV(GPUBufferComponent* gpuBuffer);
		bool CreateCBV(GPUBufferComponent* gpuBuffer);
		bool UploadToGPU(CommandListComponent* commandList, DX12MappedMemory* mappedMemory, DX12DeviceMemory* deviceMemory, GPUBufferComponent* gpuBuffer);

		DX12Context* m_ctx = nullptr;
	};
}

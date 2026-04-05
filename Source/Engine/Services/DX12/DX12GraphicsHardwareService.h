#pragma once
#include "../GraphicsHardwareService.h"
#include "DX12Context.h"

namespace Inno
{
	class DX12GraphicsHardwareService : public GraphicsHardwareService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12GraphicsHardwareService);

		DX12Context* GetDX12Context() { return &m_DX12Context; }

		bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) override;
		bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) override;
		bool Execute(CommandListComponent* commandList, GPUEngineType queueType) override;
		uint64_t GetSemaphoreValue(GPUEngineType queueType) override;
		bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) override;

		// Debug/capture
		bool BeginCapture() override;
		bool EndCapture() override;
		bool HasGPUError() const override;

		// DX12-specific public accessors (for ImGui, window surfaces, etc.)
		ComPtr<ID3D12Device8> GetDevice();
		ComPtr<ID3D12CommandAllocator> GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
		ComPtr<ID3D12CommandQueue> GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
		DX12DescriptorHeapAccessor& GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility = Accessibility::ReadOnly,
			Accessibility resourceAccessibility = Accessibility::ReadOnly, TextureUsage textureUsage = TextureUsage::Invalid, bool isShaderVisible = true);

	protected:
		bool CreateHardwareResources() override;
		bool ReleaseHardwareResources() override;

	private:
		// DX12 hardware initialization functions
		bool CreateDebugCallback();
		bool CreatePhysicalDevices();
		bool CreateGlobalCommandQueues();
		bool CreateGlobalCommandAllocators();
		bool CreateSyncPrimitives();
		bool CreateGlobalDescriptorHeaps();

		template <typename U, typename T>
		bool SetObjectName(U* owner, const T& rhs, const char* objectTypeSuffix);

		// DX12 context (owned by this service, shared with other DX12 services)
		DX12Context m_DX12Context;
	};
}

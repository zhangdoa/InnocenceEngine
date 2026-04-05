#pragma once
#include "../FrameManagementService.h"
#include "DX12Context.h"

namespace Inno
{
	class DX12GraphicsResourceService;

	class DX12FrameManagementService : public FrameManagementService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12FrameManagementService);

		DX12Context* GetDX12Context() { return &m_DX12Context; }

	protected:
		// DX12-specific frame lifecycle
		bool BeginFrame() override;
		bool EndFrame() override;
		bool PresentImpl() override;
		bool ResizeImpl() override;
		bool WaitAllOnCPU() override;

		// DX12-specific swap chain
		bool GetSwapChainImages() override;
		bool AssignSwapChainImages() override;
		bool ReleaseSwapChainImages() override;

		// DX12-specific raytracing prep
		bool PrepareRayTracing(CommandListComponent* commandList) override;

		// DX12-specific hardware init/teardown
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
		bool CreateSwapChain();

		template <typename U, typename T>
		bool SetObjectName(U* owner, const T& rhs, const char* objectTypeSuffix);

		// DX12 context (owned by this service, shared with other DX12 services)
		DX12Context m_DX12Context;

		// Swap chain
		std::vector<ComPtr<ID3D12Resource>> m_swapChainImages;
		DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc = {};
		ComPtr<IDXGISwapChain4> m_swapChain = nullptr;

		// Debug capture
		bool m_BeginCapture = false;
		bool m_EndCapture = false;
	};
}

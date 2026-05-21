#pragma once
#include "DX12Headers.h"

namespace Inno
{
	struct DX12Context
	{
		int32_t m_videoCardMemory = 0;
		char m_videoCardDescription[128] = {};

		ComPtr<ID3D12Debug1> m_debugInterface = nullptr;
		ComPtr<IDXGraphicsAnalysis> m_graphicsAnalysis = nullptr;
		DWORD m_debugCallbackCookie = 0;

		ComPtr<IDXGIFactory7> m_factory = nullptr;
		DXGI_ADAPTER_DESC m_adapterDesc = {};
		ComPtr<IDXGIAdapter4> m_adapter = nullptr;
		ComPtr<IDXGIOutput6> m_adapterOutput = nullptr;

		ComPtr<ID3D12Device9> m_device = nullptr;

		ComPtr<ID3D12CommandQueue> m_directCommandQueue = nullptr;
		ComPtr<ID3D12CommandQueue> m_computeCommandQueue = nullptr;
		ComPtr<ID3D12CommandQueue> m_copyCommandQueue = nullptr;

		ComPtr<ID3D12Fence> m_directCommandQueueFence = nullptr;
		ComPtr<ID3D12Fence> m_computeCommandQueueFence = nullptr;
		ComPtr<ID3D12Fence> m_copyCommandQueueFence = nullptr;

		std::vector<ComPtr<ID3D12CommandAllocator>> m_directCommandAllocators;
		std::vector<ComPtr<ID3D12CommandAllocator>> m_computeCommandAllocators;
		std::vector<ComPtr<ID3D12CommandAllocator>> m_copyCommandAllocators;

		ComPtr<ID3D12DescriptorHeap> m_CSUDescHeap = nullptr;
		DX12DescriptorHeapAccessor m_GPUBuffer_CBV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_GPUBuffer_SRV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_MaterialTexture_SRV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_RenderTarget_SRV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_GPUBuffer_UAV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_MaterialTexture_UAV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_RenderTarget_UAV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_BindlessMeshVertex_SRV_DescHeapAccessor;
		DX12DescriptorHeapAccessor m_BindlessMeshIndex_SRV_DescHeapAccessor;

		ComPtr<ID3D12DescriptorHeap> m_CSUDescHeap_ShaderNonVisible = nullptr;
		DX12DescriptorHeapAccessor m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible;
		DX12DescriptorHeapAccessor m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible;
		DX12DescriptorHeapAccessor m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible;

		ComPtr<ID3D12DescriptorHeap> m_RTVDescHeap = nullptr;
		DX12DescriptorHeapAccessor m_RTVDescHeapAccessor;
		ComPtr<ID3D12DescriptorHeap> m_DSVDescHeap = nullptr;
		DX12DescriptorHeapAccessor m_DSVDescHeapAccessor;
		ComPtr<ID3D12DescriptorHeap> m_SamplerDescHeap = nullptr;
		DX12DescriptorHeapAccessor m_SamplerDescHeapAccessor;

		mutable std::atomic<bool> m_GPUErrorDetected{false};

		ComPtr<ID3D12QueryHeap> m_TimestampHeap_Graphics = nullptr;
		ComPtr<ID3D12QueryHeap> m_TimestampHeap_Compute = nullptr;
		ComPtr<ID3D12QueryHeap> m_TimestampHeap_Copy = nullptr;
		// One per swapchain image, sized for 2 * GPU_TIMER_MAX_NAMED_TIMERS
		// UINT64s per queue (begin + end).
		std::vector<ComPtr<ID3D12Resource>> m_TimestampReadback_Graphics;
		std::vector<ComPtr<ID3D12Resource>> m_TimestampReadback_Compute;
		std::vector<ComPtr<ID3D12Resource>> m_TimestampReadback_Copy;
		// Dedicated per-frame allocators/lists for ResolveQueryData; sharing the
		// global pass allocator would race with the in-flight pass list still
		// recording when ResolveGpuTimers fires.
		std::vector<ComPtr<ID3D12CommandAllocator>> m_TimestampResolveAllocators_Graphics;
		std::vector<ComPtr<ID3D12CommandAllocator>> m_TimestampResolveAllocators_Compute;
		std::vector<ComPtr<ID3D12CommandAllocator>> m_TimestampResolveAllocators_Copy;
		std::vector<ComPtr<ID3D12GraphicsCommandList7>> m_TimestampResolveLists_Graphics;
		std::vector<ComPtr<ID3D12GraphicsCommandList7>> m_TimestampResolveLists_Compute;
		std::vector<ComPtr<ID3D12GraphicsCommandList7>> m_TimestampResolveLists_Copy;

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		static constexpr bool m_enableValidationLayers = true;
#else
		static constexpr bool m_enableValidationLayers = false;
#endif

		ComPtr<ID3D12CommandAllocator> GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, uint32_t frameIndex);
		ComPtr<ID3D12CommandQueue> GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
		DX12DescriptorHeapAccessor& GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility = Accessibility::ReadOnly,
			Accessibility resourceAccessibility = Accessibility::ReadOnly, TextureUsage textureUsage = TextureUsage::Invalid, bool isShaderVisible = true);

		ComPtr<ID3D12Resource> CreateUploadHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, const char* name = "");
		ComPtr<ID3D12Resource> CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, D3D12_CLEAR_VALUE* clearValue = nullptr, bool isShared = false, const char* name = "");
		ComPtr<ID3D12Resource> CreateReadBackHeapBuffer(UINT64 size, const char* name = "");
		ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_QUEUE_DESC* commandQueueDesc, const wchar_t* name = L"");
		ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, const wchar_t* name = L"");
		ComPtr<ID3D12GraphicsCommandList7> CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator, const wchar_t* name = L"");
		ComPtr<ID3D12GraphicsCommandList7> CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator);
		ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc, const wchar_t* name = L"");

		DX12DescriptorHeapAccessor CreateDescriptorHeapAccessor(
			ComPtr<ID3D12DescriptorHeap> descHeap,
			D3D12_DESCRIPTOR_HEAP_DESC desc,
			uint32_t maxDescriptors,
			uint32_t descriptorSize,
			const DescriptorHandle& firstHandle,
			bool shaderVisible,
			const wchar_t* name);
	};
}

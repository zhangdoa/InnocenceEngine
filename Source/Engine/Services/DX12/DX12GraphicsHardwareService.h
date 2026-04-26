#pragma once
#include "../GraphicsHardwareService.h"
#include "DX12Context.h"

namespace Inno
{
	// One per-queue book-keeping slot for an in-flight GPU timer. The slot index is
	// stable for the lifetime of the named timer so begin / end / resolve / readback
	// all address the same two query slots.
	struct DX12GpuTimerSlot
	{
		std::string m_Name;
		uint32_t m_SlotIndex = 0;          // 0..GPU_TIMER_MAX_NAMED_TIMERS-1; queries live at slot*2 (begin) and slot*2+1 (end).
		bool m_BeginRecorded = false;       // Set by BeginGpuTimer; cleared by EndGpuTimer. Detects unmatched calls.
		bool m_EndRecordedThisFrame = false; // Set by EndGpuTimer; cleared by ResolveGpuTimers. Distinguishes "fully recorded this frame" from "begun but never ended".
	};

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
		void DumpGPUDiagnostics() override;

		// GPU timestamp queries
		bool BeginGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType) override;
		bool EndGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType) override;
		bool ResolveGpuTimers() override;
		std::vector<GpuTimingResult> GetGpuTimings() const override;

		// PIX event markers (dynamic-loaded WinPixEventRuntime)
		bool BeginGpuEvent(CommandListComponent* commandList, const char* name, uint32_t color = 0) override;
		bool EndGpuEvent(CommandListComponent* commandList) override;

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
		bool CreateGpuTimerResources();
		void ReleaseGpuTimerResources();

		template <typename U, typename T>
		bool SetObjectName(U* owner, const T& rhs, const char* objectTypeSuffix);

		bool TryLoadRenderDocAPI();
		void TryLoadPIXEventRuntime();

		// Fence wait with rich diagnostics on timeout/failure (TASK-34).
		bool WaitOnFenceWithDiagnostics(const char* fenceName, ID3D12Fence* fence, HANDLE fenceEvent, uint64_t semaphoreValue);

		// Per-queue timer state. Vectors are pre-sized at startup; FindOrAllocateTimer
		// consults / mutates only the matching queue's vector. Caller (frame thread)
		// serialises Begin/End/Resolve so there's no internal lock.
		struct DX12GpuTimerQueueState
		{
			std::vector<DX12GpuTimerSlot> m_Slots;             // Indexed by slot; m_Slots.size() == nextFree allocator.
			std::vector<GpuTimingResult> m_LatestTimings;      // Filled by ResolveGpuTimers from the readback buffer N frames behind.
			D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
			// Highest slot index for which BOTH Begin and End queries have been
			// recorded at least once across the run. Resolve range is bounded by
			// this because D3D12 errors on ResolveQueryData for never-performed
			// queries (validated by GBV).
			int32_t m_MaxEverFullyRecordedSlot = -1;
		};

		DX12GpuTimerQueueState* GetTimerState(GPUEngineType queueType);
		const DX12GpuTimerQueueState* GetTimerState(GPUEngineType queueType) const;
		ComPtr<ID3D12QueryHeap> GetTimestampHeap(GPUEngineType queueType) const;
		ComPtr<ID3D12Resource> GetTimestampReadback(GPUEngineType queueType, uint32_t frameIndex) const;
		// Returns slot index, or UINT32_MAX on capacity-exhausted (logged loudly).
		uint32_t FindOrAllocateTimerSlot(GPUEngineType queueType, const char* name);
		// Returns slot index for an existing name, or UINT32_MAX if not found (logged loudly).
		uint32_t FindTimerSlot(GPUEngineType queueType, const char* name) const;

		// DX12 context (owned by this service, shared with other DX12 services)
		DX12Context m_DX12Context;

		void* m_RenderDocAPI = nullptr;

		// PIX event runtime — function pointers resolved at startup from
		// WinPixEventRuntime.dll. Null when the runtime is not loaded; all
		// BeginGpuEvent/EndGpuEvent calls become cheap no-ops in that case.
		void* m_PIXModule = nullptr;
		using PIXBeginEventOnCommandListFn = void(*)(void*, uint64_t, const char*);
		using PIXEndEventOnCommandListFn   = void(*)(void*);
		PIXBeginEventOnCommandListFn m_PIXBeginEventOnCommandList = nullptr;
		PIXEndEventOnCommandListFn   m_PIXEndEventOnCommandList   = nullptr;

		// GPU timer state, one entry per queue type (Graphics, Compute, Copy).
		DX12GpuTimerQueueState m_TimerState_Graphics;
		DX12GpuTimerQueueState m_TimerState_Compute;
		DX12GpuTimerQueueState m_TimerState_Copy;
		// Tracks which swapchain frame slot the next ResolveGpuTimers call should
		// resolve INTO. Readback is GPU_TIMER_READBACK_FRAME_LATENCY frames behind.
		uint64_t m_TimerResolveFrameCounter = 0;
	};
}

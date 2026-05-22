#pragma once
#include "../../Common/Array.h"
#include "../GraphicsHardwareService.h"
#include "DX12Context.h"

namespace Inno
{
	// Slot index is stable for the lifetime of the named timer so begin / end /
	// resolve / readback all address the same query pair.
	struct DX12GpuTimerSlot
	{
		std::string m_Name;
		uint32_t m_SlotIndex = 0;          // queries live at slot*2 (begin) and slot*2+1 (end).
		bool m_BeginRecorded = false;
		bool m_EndRecordedThisFrame = false; // Distinguishes "fully recorded this frame" from "begun but never ended".
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

		bool BeginCapture() override;
		bool EndCapture() override;
		bool HasGPUError() const override;
		void DumpGPUDiagnostics() override;

		bool BeginGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType) override;
		bool EndGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType) override;
		bool ResolveGpuTimers() override;
		Inno::Array<GpuTimingResult> GetGpuTimings() const override;

		bool BeginGpuEvent(CommandListComponent* commandList, const char* name, uint32_t color = 0) override;
		bool EndGpuEvent(CommandListComponent* commandList) override;

		ComPtr<ID3D12Device8> GetDevice();
		ComPtr<ID3D12CommandAllocator> GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
		ComPtr<ID3D12CommandQueue> GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
		DX12DescriptorHeapAccessor& GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility = Accessibility::ReadOnly,
			Accessibility resourceAccessibility = Accessibility::ReadOnly, TextureUsage textureUsage = TextureUsage::Invalid, bool isShaderVisible = true);

	protected:
		bool CreateHardwareResources() override;
		bool ReleaseHardwareResources() override;

	private:
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

		bool WaitOnFenceWithDiagnostics(const char* fenceName, ID3D12Fence* fence, HANDLE fenceEvent, uint64_t semaphoreValue);

		// Caller (frame thread) serialises Begin/End/Resolve, so the per-queue state
		// carries no internal lock.
		struct DX12GpuTimerQueueState
		{
			Inno::Array<DX12GpuTimerSlot> m_Slots;
			Inno::Array<GpuTimingResult> m_LatestTimings;
			D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
			// D3D12 GBV rejects ResolveQueryData over queries that were never
			// performed, so the resolve range is bounded by this watermark.
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

		DX12Context m_DX12Context;

		void* m_RenderDocAPI = nullptr;

		// Null when WinPixEventRuntime.dll did not load; BeginGpuEvent / EndGpuEvent
		// become no-ops in that case.
		void* m_PIXModule = nullptr;
		using PIXBeginEventOnCommandListFn = void(*)(void*, uint64_t, const char*);
		using PIXEndEventOnCommandListFn   = void(*)(void*);
		PIXBeginEventOnCommandListFn m_PIXBeginEventOnCommandList = nullptr;
		PIXEndEventOnCommandListFn   m_PIXEndEventOnCommandList   = nullptr;

		DX12GpuTimerQueueState m_TimerState_Graphics;
		DX12GpuTimerQueueState m_TimerState_Compute;
		DX12GpuTimerQueueState m_TimerState_Copy;
		// Readback is GPU_TIMER_READBACK_FRAME_LATENCY frames behind the resolve.
		uint64_t m_TimerResolveFrameCounter = 0;
	};
}

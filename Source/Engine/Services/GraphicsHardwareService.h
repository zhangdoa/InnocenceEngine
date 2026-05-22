#pragma once
#include "../Common/Array.h"
#include "../Interface/IService.h"
#include "../Common/GraphicsPrimitive.h"
#include "../Component/RenderPassComponent.h"

namespace Inno
{
	class CommandListComponent;
	struct GPUResourceComponent;
	class TextureComponent;
	class GPUBufferComponent;
	class MeshComponent;
	class FrameManagementService;

	struct GpuTimingResult
	{
		std::string m_Name;
		double m_Milliseconds = 0.0;
		GPUEngineType m_QueueType = GPUEngineType::Graphics;
	};

	class GraphicsHardwareService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(GraphicsHardwareService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override { return true; }
		ObjectStatus GetStatus() override { return ObjectStatus::Activated; }

		void SetFrameManagementService(FrameManagementService* fmService) { m_FrameManagementService = fmService; }

		// Sync primitives (low-level)
		virtual bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) = 0;
		virtual bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) = 0;
		virtual bool Execute(CommandListComponent* commandList, GPUEngineType queueType) = 0;
		virtual uint64_t GetSemaphoreValue(GPUEngineType queueType) = 0;
		virtual bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) = 0;

		// Sync primitives (RenderPass convenience)
		bool SignalOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType);
		bool WaitOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType, GPUEngineType semaphoreType);

		// Debug/capture
		virtual bool BeginCapture() { return false; }
		virtual bool EndCapture() { return false; }
		virtual bool HasGPUError() const { return false; }
		virtual void DumpGPUDiagnostics() {}

		// GPU timestamp queries — wrap a pass to measure GPU-side cost.
		// Begin/End must be paired with the same name on the same queue type.
		// Mismatched calls are logged loudly (Warning), not silently dropped.
		// Recorded into the supplied command list at record time; resolved
		// once per frame (ResolveGpuTimers) into a frame-latency-delayed
		// readback buffer. GetGpuTimings returns the most recent fully
		// readable frame's results; the per-pass cost may lag by a few frames
		// to avoid CPU↔GPU sync stalls.
		// Thread-safety contract: caller serialises Begin/End/Resolve per
		// queue (the engine's frame loop already does — they run on the
		// frame thread). GetGpuTimings is read-only and may be called from
		// any thread once per frame.
		virtual bool BeginGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType) { return false; }
		virtual bool EndGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType) { return false; }
		virtual bool ResolveGpuTimers() { return false; }
		virtual Inno::Array<GpuTimingResult> GetGpuTimings() const { return {}; }

		// PIX event markers — named events on the PIX timeline. Macros are
		// zero-cost when the WinPixEventRuntime DLL isn't loaded into the
		// process (pre-injected by PIX or LoadLibrary'd at startup).
		// Begin/End must be paired on the same command list.
		virtual bool BeginGpuEvent(CommandListComponent* commandList, const char* name, uint32_t color = 0) { return false; }
		virtual bool EndGpuEvent(CommandListComponent* commandList) { return false; }

		// Convenience wrapper: BeginGpuEvent(name) + BeginGpuTimer(name).
		// Use in render-pass PrepareCommandList around Dispatch/DrawIndexed
		// to get both PIX-timeline naming and per-pass GPU timing in one call.
		bool BeginGpuPass(CommandListComponent* commandList, const char* name, GPUEngineType queueType, uint32_t color = 0);
		bool EndGpuPass(CommandListComponent* commandList, const char* name, GPUEngineType queueType);

	protected:
		virtual bool CreateHardwareResources() { return true; }
		virtual bool ReleaseHardwareResources() { return true; }

		FrameManagementService* m_FrameManagementService = nullptr;
	};
}

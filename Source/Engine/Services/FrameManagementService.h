#pragma once
#include "../Interface/IService.h"
#include "../Common/GraphicsPrimitive.h"
#include "../Common/MathHelper.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/CommandListComponent.h"

namespace Inno
{
	class GraphicsHardwareService;
	struct GPUResourceComponent;
	class TextureComponent;
	class GPUBufferComponent;
	class MeshComponent;

	class FrameManagementService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(FrameManagementService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		void SetHardwareService(GraphicsHardwareService* hardwareService) { m_HardwareService = hardwareService; }

		// Frame queries (concrete - reads local data)
		uint32_t GetCurrentFrame();
		uint32_t GetPreviousFrame();
		uint32_t GetNextFrame();
		uint32_t GetSwapChainImageCount();
		uint32_t GetFrameCountSinceLaunch();

		// TASK-213 CL A: scene-readiness predicate for auto-test capture
		// determinism. Returns true iff (a) MeshResourceService deferred-
		// activation queue is empty, (b) TLAS instance count has been stable
		// for K=3 consecutive frames (no UpdateRaytracingInstances rebuild),
		// and (c) SceneService is not loading. Pure read of state — no work
		// done, no timer/sleep, signal-driven.
		// Mutates internal rolling state (K-window counter, latch flags) — call
		// from exactly one place per frame; the dispatch site is FMS::Update().
		bool IsSteadyState();

		// TASK-213 CL B: latch + steady-state-relative frame counter consumed
		// by capture-mode determinism.
		// HasReachedSteadyState(): true iff IsSteadyState() has gone true at
		//   least once this session. Latches once-true-stays-true (does not
		//   clear on TLAS-instance flap-back). Pure read, safe to call from
		//   anywhere.
		// GetSteadyStateRelativeFrameCount(): 0 until the first-true latch,
		//   then advances 1-per-frame from that point. Used as the dump-frame
		//   index (`gpu_output_NNNN.png`) and as the PT-RNG seed source in
		//   capture mode so both are deterministic per scene across launches
		//   regardless of variable load-frame counts.
		bool HasReachedSteadyState() const { return m_SteadyStateMarkerLogged; }
		uint32_t GetSteadyStateRelativeFrameCount() const;

		// Callbacks from Engine
		void SetUploadHeapPreparationCallback(std::function<bool()>&& callback);
		void SetCommandPreparationCallback(std::function<bool()>&& callback);
		void SetCommandExecutionCallback(std::function<bool()>&& callback);
		// Pre/post-frame hooks so clients (e.g. the rendering client's capture
		// logic, auto-test instrumentation) can run work at frame boundaries
		// without FrameManagementService having to know about the feature.
		void SetPreFrameCallback(std::function<void(uint32_t)>&& callback);
		void SetPostFrameCallback(std::function<void(uint32_t)>&& callback);

		// Swap chain
		RenderPassComponent* GetSwapChainRenderPassComponent();
		bool Resize();
		bool Present();

		// User pipeline output
		bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& func);
		GPUResourceComponent* GetUserPipelineOutput();

		// Cross-service access
		ISemaphore* GetGlobalSemaphore();
		void SetGlobalSemaphore(ISemaphore* semaphore) { m_GlobalSemaphore = semaphore; }
		std::vector<CommandListComponent*>& GetGlobalGraphicsCommandLists() { return m_GlobalGraphicsCommandLists; }

		// Command list lifecycle
		virtual bool Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject = nullptr) { return false; }
		virtual bool Close(CommandListComponent* commandList, GPUEngineType engineType) { return false; }

		// Command recording
		virtual bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex) { return false; }
		virtual bool BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList) { return false; }
		virtual bool ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index = SIZE_MAX) { return false; }
		virtual bool BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual bool TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) { return false; }
		virtual bool TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) { return false; }
		virtual bool DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount = 1) { return false; }
		virtual bool DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount = 1) { return false; }
		virtual bool Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) { return false; }
		virtual bool DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ) { return false; }
		virtual bool ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand) { return false; }
		virtual void PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants) {}
		virtual bool CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList) { return false; }

		bool WaitForGPUIdle();

	protected:
		virtual bool CreateSwapChainResources() { return true; }
		virtual bool BeginFrame() { return false; }
		virtual bool EndFrame() { return false; }
		virtual bool PresentImpl() { return false; }
		virtual bool ResizeImpl() { return false; }
		virtual bool WaitAllOnCPU() { return false; }
		virtual bool GetSwapChainImages() { return false; }
		virtual bool AssignSwapChainImages() { return false; }
		virtual bool ReleaseSwapChainImages() { return false; }
		virtual bool PrepareRayTracing(CommandListComponent* commandList) { return false; }

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		GraphicsHardwareService* m_HardwareService = nullptr;

		// Frame counters (moved from IGraphicsService)
		uint32_t m_swapChainImageCount = 3;
		uint32_t m_CurrentFrame = 0;
		std::atomic<uint32_t> m_FrameCountSinceLaunch = 0;
		TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);

		// Semaphore tracking per frame
		std::vector<uint64_t> m_GraphicsSemaphoreValues;
		std::vector<uint64_t> m_ComputeSemaphoreValues;
		std::vector<uint64_t> m_CopySemaphoreValues;

		// Global command infrastructure
		std::vector<CommandListComponent*> m_GlobalGraphicsCommandLists;
		ISemaphore* m_GlobalSemaphore = nullptr;

		// Swap chain render pass
		RenderPassComponent* m_SwapChainRenderPassComp = nullptr;

		// Callbacks
		std::function<GPUResourceComponent* ()> m_GetUserPipelineOutputFunc;

		// TASK-213 CL A: IsSteadyState() rolling state.
		// Last observed TLAS instance count and consecutive-frames-stable
		// counter — incremented when GetRaytracingInstanceCount() matches the
		// previous observation, reset to 0 on change. K=3 chosen because
		// GISponza's TLAS log shows count flips at frame=0/1/2/16/30; a single
		// stable frame is insufficient to ride out the late-binding rebuilds
		// driven by deferred-mesh activation (RC-1 in the task design pass).
		size_t m_LastObservedInstanceCount = SIZE_MAX;
		uint32_t m_TLASStableFrameCount = 0;
		// Latches once the predicate first goes true so the marker logs once.
		bool m_SteadyStateMarkerLogged = false;
		// 120-frame timeout watchdog (R2 in the design pass risk register):
		// if the predicate never goes true, log a Warning so a future capture
		// missing the steady-state marker is loud, not silent. The script-
		// level -total_frames cap remains the hard-stop.
		bool m_SteadyStateTimeoutLogged = false;
		// TASK-213 CL B: m_FrameCountSinceLaunch value at the frame the marker
		// first latched. GetSteadyStateRelativeFrameCount() reads
		// m_FrameCountSinceLaunch - m_FirstSteadyStateFrame; SIZE_MAX-equivalent
		// sentinel (UINT32_MAX) means "not yet latched", which the accessor maps
		// to 0.
		uint32_t m_FirstSteadyStateFrame = UINT32_MAX;

	private:
		bool InitializeSwapChainRenderPassComponent();
		bool PrepareGlobalCommands();
		bool ExecuteGlobalCommands();
		bool PrepareSwapChainCommands();
		bool ExecuteSwapChainCommands();
		bool ExecuteResize();
		bool PreResize();
		bool PreResize(RenderPassComponent* renderPass);
		bool PostResize();
		bool PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* renderPass);

		// Swap chain components (owned by this service)
		ShaderProgramComponent* m_SwapChainShaderProgramComp = nullptr;
		SamplerComponent* m_SwapChainSamplerComp = nullptr;

		std::function<bool()> m_UploadHeapPreparationCallback;
		std::function<bool()> m_CommandPreparationCallback;
		std::function<bool()> m_CommandExecutionCallback;
		std::function<void(uint32_t)> m_PreFrameCallback;
		std::function<void(uint32_t)> m_PostFrameCallback;

		std::atomic_bool m_needResize = false;
		bool m_DeviceErrorReported = false;
	};
}

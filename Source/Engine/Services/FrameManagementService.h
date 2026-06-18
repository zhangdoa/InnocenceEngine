#pragma once
#include "../Common/Array.h"
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
		INNO_CLASS_INTERFACE_NON_COPYABLE_AND_NON_MOVABLE(FrameManagementService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		void SetHardwareService(GraphicsHardwareService* hardwareService) { m_HardwareService = hardwareService; }

		uint32_t GetCurrentFrame();
		uint32_t GetPreviousFrame();
		uint32_t GetNextFrame();
		uint32_t GetSwapChainImageCount();
		uint32_t GetFrameCountSinceLaunch();

		// True iff (a) MeshResourceService deferred-activation queue is empty,
		// (b) TLAS instance count has been stable for K=3 consecutive frames,
		// and (c) SceneService is not loading. Mutates internal rolling state
		// (K-window counter, latch flags) — call exactly once per frame from FMS::Update().
		bool IsSteadyState();

		// Single session clock anchored at m_FirstSteadyStateFrame, latched once
		// (never reset) when steady state is first detected OR the watchdog times
		// out. Stays false/0 until the anchor; then the relative count advances
		// 1-per-frame. All frame-indexed lifecycle triggers + the PT-RNG seed
		// measure from it — capture timing is load-count-independent.
		bool HasReachedSteadyState() const { return m_FirstSteadyStateFrame != UINT32_MAX; }
		uint32_t GetSteadyStateRelativeFrameCount() const;

		void SetUploadHeapPreparationCallback(std::function<bool()>&& callback);
		void SetCommandPreparationCallback(std::function<bool()>&& callback);
		void SetCommandExecutionCallback(std::function<bool()>&& callback);
		void SetPreFrameCallback(std::function<void(uint32_t)>&& callback);
		void SetPostFrameCallback(std::function<void(uint32_t)>&& callback);

		RenderPassComponent* GetSwapChainRenderPassComponent();
		bool Resize();
		bool Present();

		GPUResourceComponent* GetUserPipelineOutput();

		ISemaphore* GetGlobalSemaphore();
		void SetGlobalSemaphore(ISemaphore* semaphore) { m_GlobalSemaphore = semaphore; }
		Inno::Array<CommandListComponent*>& GetGlobalGraphicsCommandLists() { return m_GlobalGraphicsCommandLists; }

		virtual bool Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject = nullptr) { return false; }
		virtual bool Close(CommandListComponent* commandList, GPUEngineType engineType) { return false; }

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

		uint32_t m_swapChainImageCount = 3;
		uint32_t m_CurrentFrame = 0;
		std::atomic<uint32_t> m_FrameCountSinceLaunch = 0;
		TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);

		Inno::Array<uint64_t> m_GraphicsSemaphoreValues;
		Inno::Array<uint64_t> m_ComputeSemaphoreValues;
		Inno::Array<uint64_t> m_CopySemaphoreValues;

		Inno::Array<CommandListComponent*> m_GlobalGraphicsCommandLists;
		ISemaphore* m_GlobalSemaphore = nullptr;

		RenderPassComponent* m_SwapChainRenderPassComp = nullptr;



		// K=3 frames because GISponza's TLAS log shows count flips at frame=0/1/2/16/30;
		// a single stable frame does not ride out late-binding rebuilds driven by
		// deferred-mesh activation.
		size_t m_LastObservedInstanceCount = SIZE_MAX;
		uint32_t m_TLASStableFrameCount = 0;
		bool m_SteadyStateMarkerLogged = false;
		// 120-frame timeout watchdog: if the predicate never goes true, log a Warning
		// so a capture missing the steady-state marker is loud rather than silent.
		bool m_SteadyStateTimeoutLogged = false;
		// FCSL value at the latch frame; UINT32_MAX sentinel = not yet latched
		// (accessor maps to 0).
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

		// Frame budget: request a clean shutdown once totalFrames session-frames
		// have been rendered. The universal terminate path for every mode.
		void EvaluateFrameBudget();

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

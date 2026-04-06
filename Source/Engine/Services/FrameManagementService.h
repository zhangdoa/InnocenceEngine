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

		// Callbacks from Engine
		void SetUploadHeapPreparationCallback(std::function<bool()>&& callback);
		void SetCommandPreparationCallback(std::function<bool()>&& callback);
		void SetCommandExecutionCallback(std::function<bool()>&& callback);

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
		virtual void* GetViewportSharedHandle() { return nullptr; }

		// Command list lifecycle
		virtual bool Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject = nullptr) { return false; }
		virtual bool Close(CommandListComponent* commandList, GPUEngineType engineType) { return false; }

		// Command recording
		virtual bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex) { return false; }
		virtual bool BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList) { return false; }
		virtual bool ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index = SIZE_MAX) { return false; }
		virtual bool BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual bool UnbindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
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

		std::atomic_bool m_needResize = false;
	};
}

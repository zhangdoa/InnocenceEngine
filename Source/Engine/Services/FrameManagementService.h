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
	class GraphicsResourceService;
	class GraphicsHardwareService;
	struct GPUResourceComponent;

	class FrameManagementService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(FrameManagementService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		void SetResourceService(GraphicsResourceService* resourceService) { m_ResourceService = resourceService; }
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
		std::vector<CommandListComponent*>& GetGlobalGraphicsCommandLists() { return m_GlobalGraphicsCommandLists; }

	protected:
		// DX12-specific frame lifecycle (virtual - backend overrides)
		virtual bool BeginFrame() { return false; }
		virtual bool EndFrame() { return false; }
		virtual bool PresentImpl() { return false; }
		virtual bool ResizeImpl() { return false; }
		virtual bool WaitAllOnCPU() { return false; }

		// DX12-specific swap chain (virtual - backend overrides)
		virtual bool GetSwapChainImages() { return false; }
		virtual bool AssignSwapChainImages() { return false; }
		virtual bool ReleaseSwapChainImages() { return false; }

		// DX12-specific raytracing prep (virtual)
		virtual bool PrepareRayTracing(CommandListComponent* commandList) { return false; }

		// DX12-specific hardware init/teardown (virtual - DX12 creates device, queues, etc.)
		virtual bool CreateHardwareResources() { return true; }
		virtual bool ReleaseHardwareResources() { return true; }

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		GraphicsResourceService* m_ResourceService = nullptr;
		GraphicsHardwareService* m_HardwareService = nullptr;

		// Frame counters (moved from IGraphicsService)
		uint32_t m_swapChainImageCount = 2;
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

		// Callbacks
		std::function<GPUResourceComponent* ()> m_GetUserPipelineOutputFunc;
		std::function<bool()> m_UploadHeapPreparationCallback;
		std::function<bool()> m_CommandPreparationCallback;
		std::function<bool()> m_CommandExecutionCallback;

		std::atomic_bool m_needResize = false;
	};
}

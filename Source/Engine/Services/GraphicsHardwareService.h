#pragma once
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

	protected:
		virtual bool CreateHardwareResources() { return true; }
		virtual bool ReleaseHardwareResources() { return true; }

		FrameManagementService* m_FrameManagementService = nullptr;
	};
}

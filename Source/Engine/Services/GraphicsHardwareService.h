#pragma once
#include "../Interface/IService.h"
#include "../Common/GraphicsPrimitive.h"
#include "../Component/RenderPassComponent.h"

namespace Inno
{
	class CommandListComponent;

	class GraphicsHardwareService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(GraphicsHardwareService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override { return true; }
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override { return true; }
		ObjectStatus GetStatus() override { return ObjectStatus::Activated; }

		// Sync primitives (low-level)
		virtual bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) = 0;
		virtual bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) = 0;
		virtual bool Execute(CommandListComponent* commandList, GPUEngineType queueType) = 0;
		virtual uint64_t GetSemaphoreValue(GPUEngineType queueType) = 0;
		virtual bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) = 0;

		// Sync primitives (RenderPass convenience)
		bool SignalOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType);
		bool WaitOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType, GPUEngineType semaphoreType);

		// Command list lifecycle
		virtual bool Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject = nullptr) = 0;
		virtual bool Close(CommandListComponent* commandList, GPUEngineType engineType) = 0;

		// Debug/capture
		virtual bool BeginCapture() { return false; }
		virtual bool EndCapture() { return false; }
		virtual bool HasGPUError() const { return false; }
	};
}

#pragma once
#include "../GraphicsHardwareService.h"

namespace Inno
{
	class IGraphicsService;

	class DX12GraphicsHardwareService : public GraphicsHardwareService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12GraphicsHardwareService);

		void SetBackend(IGraphicsService* backend) { m_Backend = backend; }

		bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) override;
		bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) override;
		bool Execute(CommandListComponent* commandList, GPUEngineType queueType) override;
		uint64_t GetSemaphoreValue(GPUEngineType queueType) override;
		bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) override;

		bool Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject = nullptr) override;
		bool Close(CommandListComponent* commandList, GPUEngineType engineType) override;

		bool BeginCapture() override;
		bool EndCapture() override;
		bool HasGPUError() const override;

	private:
		IGraphicsService* m_Backend = nullptr;
	};
}

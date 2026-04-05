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

		// Command recording
		virtual bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex) = 0;
		virtual bool BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList) = 0;
		virtual bool ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index = SIZE_MAX) = 0;
		virtual bool BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) = 0;
		virtual bool UnbindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) = 0;
		virtual bool TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) = 0;
		virtual bool TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) = 0;
		virtual bool DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount = 1) = 0;
		virtual bool DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount = 1) = 0;
		virtual bool Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) = 0;
		virtual bool DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ) = 0;
		virtual bool ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand) = 0;
		virtual void PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants) = 0;
		virtual bool CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList) = 0;

		// Debug/capture
		virtual bool BeginCapture() { return false; }
		virtual bool EndCapture() { return false; }
		virtual bool HasGPUError() const { return false; }
	};
}

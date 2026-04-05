#pragma once
#include "../GraphicsHardwareService.h"
#include "DX12Context.h"

namespace Inno
{
	class DX12GraphicsHardwareService : public GraphicsHardwareService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12GraphicsHardwareService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) override;
		bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) override;
		bool Execute(CommandListComponent* commandList, GPUEngineType queueType) override;
		uint64_t GetSemaphoreValue(GPUEngineType queueType) override;
		bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) override;

		bool Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject = nullptr) override;
		bool Close(CommandListComponent* commandList, GPUEngineType engineType) override;

		// Command recording
		bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex) override;
		bool BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList) override;
		bool ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index = SIZE_MAX) override;
		bool BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool UnbindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) override;
		bool TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) override;
		bool DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount = 1) override;
		bool DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount = 1) override;
		bool Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;
		bool DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ) override;
		bool ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand) override;
		void PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants) override;
		bool CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList) override;

		// Debug/capture
		bool BeginCapture() override;
		bool EndCapture() override;
		bool HasGPUError() const override;

		// DX12-specific public accessors (for ImGui, window surfaces, etc.)
		ComPtr<ID3D12Device8> GetDevice();
		ComPtr<ID3D12CommandAllocator> GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
		ComPtr<ID3D12CommandQueue> GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
		DX12DescriptorHeapAccessor& GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility = Accessibility::ReadOnly,
			Accessibility resourceAccessibility = Accessibility::ReadOnly, TextureUsage textureUsage = TextureUsage::Invalid, bool isShaderVisible = true);

	private:
		// Command recording helpers
		bool BindComputeResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource);
		bool BindGraphicsResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource);
		bool SetDescriptorHeaps(RenderPassComponent* renderPass, CommandListComponent* commandList);
		bool SetRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList);
		bool PreparePipeline(RenderPassComponent* renderPass, CommandListComponent* commandList, DX12PipelineStateObject* PSO);
		bool ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility);

		DX12Context* m_ctx = nullptr;
	};
}

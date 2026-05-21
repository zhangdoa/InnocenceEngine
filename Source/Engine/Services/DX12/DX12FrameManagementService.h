#pragma once
#include "../FrameManagementService.h"
#include "DX12Context.h"

namespace Inno
{
	class DX12FrameManagementService : public FrameManagementService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12FrameManagementService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		bool Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject = nullptr) override;
		bool Close(CommandListComponent* commandList, GPUEngineType engineType) override;

		bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex) override;
		bool BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList) override;
		bool ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index = SIZE_MAX) override;
		bool BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) override;
		bool TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) override;
		bool DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount = 1) override;
		bool DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount = 1) override;
		bool Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;
		bool DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ) override;
		bool ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand) override;
		void PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants) override;
		bool CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList) override;

	protected:
		bool CreateSwapChainResources() override;
		bool BeginFrame() override;
		bool EndFrame() override;
		bool PresentImpl() override;
		bool ResizeImpl() override;
		bool WaitAllOnCPU() override;

		bool GetSwapChainImages() override;
		bool AssignSwapChainImages() override;
		bool ReleaseSwapChainImages() override;
		bool PrepareRayTracing(CommandListComponent* commandList) override;

	private:
		bool BindComputeResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource);
		bool BindGraphicsResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource);
		bool SetDescriptorHeaps(RenderPassComponent* renderPass, CommandListComponent* commandList);
		bool SetRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList);
		bool PreparePipeline(RenderPassComponent* renderPass, CommandListComponent* commandList, DX12PipelineStateObject* PSO);
		bool ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility);

		bool CreateSwapChain();

		DX12Context* m_ctx = nullptr;

		std::vector<ComPtr<ID3D12Resource>> m_swapChainImages;
		DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc = {};
		ComPtr<IDXGISwapChain4> m_swapChain = nullptr;

		bool m_BeginCapture = false;
		bool m_EndCapture = false;
	};
}

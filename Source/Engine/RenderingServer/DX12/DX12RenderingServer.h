#pragma once
#include "../IRenderingServer.h"

namespace Inno
{
	class DX12GraphicsDevice;
	class DX12RenderingServer : public IRenderingServer
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12RenderingServer);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Terminate() override;

		// Inherited via IRenderingServer
		bool InitializeMeshComponent(MeshComponent* rhs) override;
		bool InitializeTextureComponent(TextureComponent* rhs) override;
		bool InitializeMaterialComponent(MaterialComponent* rhs) override;
		bool InitializeRenderPassComponent(RenderPassComponent* rhs) override;
		bool InitializeShaderProgramComponent(ShaderProgramComponent* rhs) override;
		bool InitializeSamplerComponent(SamplerComponent* rhs) override;
		bool InitializeGPUBufferComponent(GPUBufferComponent* rhs) override;

		bool UpdateMeshComponent(MeshComponent* rhs) override;
		bool UpdateGPUBufferComponent(GPUBufferComponent* rhs) override;

		bool ClearTextureComponent(TextureComponent* rhs) override;
		bool CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs) override;

	protected:
		virtual IRenderingGraphicsDevice* CreateRenderingGraphicsDevice() { return nullptr; }
		virtual IRenderingComponentPool* CreateRenderingComponentPool() { return nullptr; }

		bool UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range) override;

	public:		
		bool ClearGPUBufferComponent(GPUBufferComponent* rhs) override;

		void PushRootConstants(RenderPassComponent* rhs, size_t rootConstants) override;
		uint32_t GetIndex(GPUResourceComponent* rhs) override;
		bool Bind(RenderPassComponent *renderPass, GPUResourceComponent* resource, ShaderStage shaderStage, uint32_t rootParameterIndex, Accessibility bindingAccessibility) override;
		bool TryToTransitState(TextureComponent *rhs, ICommandList *commandList, Accessibility accessibility) override;

		bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex) override;
		bool BindRenderPassComponent(RenderPassComponent* rhs) override;
		bool ClearRenderTargets(RenderPassComponent* rhs, size_t index = -1) override;
		bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount) override;
		bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount) override;
		bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool CommandListEnd(RenderPassComponent* rhs) override;
		bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType) override;
		bool WaitCommandQueue(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType) override;
		bool WaitFence(RenderPassComponent* rhs, GPUEngineType GPUEngineType) override;

		bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc) override;
		GPUResourceComponent* GetUserPipelineOutput() override;

		bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;

		Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) override;
		bool GenerateMipmap(TextureComponent* rhs) override;

	private:
		DX12GraphicsDevice* m_DX12Device = nullptr;
	};
}
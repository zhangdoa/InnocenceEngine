#pragma once
#include "../IRenderingServer.h"
#include "MTRenderingServerBridge.h"

namespace Inno
{
	class MTRenderingServer : public IRenderingServer
	{
	public:
		// Inherited via IRenderingServer
		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool Initialize(MeshComponent* mesh) override;
		bool Initialize(TextureComponent* texture) override;
		bool Initialize(MaterialComponent* material) override;
		bool Initialize(RenderPassComponent* renderPass) override;
		bool Initialize(ShaderProgramComponent* shaderProgram) override;
		bool Initialize(SamplerComponent* sampler) override;
		bool Initialize(GPUBufferComponent* gpuBuffer) override;

		bool Delete(MeshComponent* mesh) override;
		bool Delete(TextureComponent* texture) override;
		bool Delete(MaterialComponent* material) override;
		bool Delete(RenderPassComponent* renderPass) override;
		bool Delete(ShaderProgramComponent* shaderProgram) override;
		bool Delete(SamplerComponent* sampler) override;
		bool Delete(GPUBufferComponent* gpuBuffer) override;

		bool Clear(TextureComponent* texture) override;
		bool Copy(TextureComponent* sourceTexture, TextureComponent* texture) override;

		bool UploadGPUBufferComponentImpl(GPUBufferComponent* gpuBuffer, const void* GPUBufferValue, size_t startOffset, size_t range) override;
		bool Clear(GPUBufferComponent* gpuBuffer) override;

		bool CommandListBegin(CommandListComponent* commandList, RenderPassComponent* renderPass, size_t frameIndex) override;
		bool BindRenderPassComponent(RenderPassComponent* renderPass) override;
		bool ClearRenderTargets(RenderPassComponent* renderPass, size_t index = -1) override;
		bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount) override;
		bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount) override;
		bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool CommandListEnd(RenderPassComponent* renderPass) override;
		bool Execute(RenderPassComponent* renderPass, GPUEngineType GPUEngineType) override;
		bool WaitForFrame(RenderPassComponent* renderPass) override;
		bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc) override;
		GPUResourceComponent* GetUserPipelineOutput() override;
		bool Present() override;

		bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;

		Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) override;
		bool GenerateMipmap(TextureComponent* texture) override;

		bool Resize() override;

		bool BeginCapture() override;
		bool EndCapture() override;

		void setBridge(MTRenderingServerBridge* bridge);
	};
}

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

		MeshComponent* AddMeshComponent(const char* name) override;
		TextureComponent* AddTextureComponent(const char* name) override;
		MaterialComponent* AddMaterialComponent(const char* name) override;
		RenderPassComponent* AddRenderPassComponent(const char* name) override;
		ShaderProgramComponent* AddShaderProgramComponent(const char* name) override;
		SamplerComponent* AddSamplerComponent(const char* name = "") override;
		GPUBufferComponent* AddGPUBufferComponent(const char* name) override;

		bool Initialize(MeshComponent* rhs) override;
		bool Initialize(TextureComponent* rhs) override;
		bool Initialize(MaterialComponent* rhs) override;
		bool Initialize(RenderPassComponent* rhs) override;
		bool Initialize(ShaderProgramComponent* rhs) override;
		bool Initialize(SamplerComponent* rhs) override;
		bool Initialize(GPUBufferComponent* rhs) override;

		bool Delete(MeshComponent* rhs) override;
		bool Delete(TextureComponent* rhs) override;
		bool Delete(MaterialComponent* rhs) override;
		bool Delete(RenderPassComponent* rhs) override;
		bool Delete(ShaderProgramComponent* rhs) override;
		bool Delete(SamplerComponent* rhs) override;
		bool Delete(GPUBufferComponent* rhs) override;

		bool Clear(TextureComponent* rhs) override;
		bool Copy(TextureComponent* lhs, TextureComponent* rhs) override;

		bool UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range) override;
		bool Clear(GPUBufferComponent* rhs) override;

		bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex) override;
		bool BindRenderPassComponent(RenderPassComponent* rhs) override;
		bool ClearRenderTargets(RenderPassComponent* rhs, size_t index = -1) override;
		bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount) override;
		bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount) override;
		bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool CommandListEnd(RenderPassComponent* rhs) override;
		bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType) override;
		bool WaitForFrame(RenderPassComponent* rhs) override;
		bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc) override;
		GPUResourceComponent* GetUserPipelineOutput() override;
		bool Present() override;

		bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;

		Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) override;
		bool GenerateMipmap(TextureComponent* rhs) override;

		bool Resize() override;

		bool BeginCapture() override;
		bool EndCapture() override;

		void setBridge(MTRenderingServerBridge* bridge);
	};
}

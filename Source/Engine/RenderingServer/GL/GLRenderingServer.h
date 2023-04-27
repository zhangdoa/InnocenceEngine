#pragma once
#include "../IRenderingServer.h"

namespace Inno
{
	class GLRenderingServer : public IRenderingServer
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

		bool InitializeMeshComponent(MeshComponent* rhs) override;
		bool InitializeTextureComponent(TextureComponent* rhs) override;
		bool InitializeMaterialComponent(MaterialComponent* rhs) override;
		bool InitializeRenderPassComponent(RenderPassComponent* rhs) override;
		bool InitializeShaderProgramComponent(ShaderProgramComponent* rhs) override;
		bool InitializeSamplerComponent(SamplerComponent* rhs) override;
		bool InitializeGPUBufferComponent(GPUBufferComponent* rhs) override;

		bool DeleteMeshComponent(MeshComponent* rhs) override;
		bool DeleteTextureComponent(TextureComponent* rhs) override;
		bool DeleteMaterialComponent(MaterialComponent* rhs) override;
		bool DeleteRenderPassComponent(RenderPassComponent* rhs) override;
		bool DeleteShaderProgramComponent(ShaderProgramComponent* rhs) override;
		bool DeleteSamplerComponent(SamplerComponent* rhs) override;
		bool DeleteGPUBufferComponent(GPUBufferComponent* rhs) override;

		bool UpdateMeshComponent(MeshComponent* rhs) override;
		
		bool ClearTextureComponent(TextureComponent* rhs) override;
		bool CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs) override;

		bool UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range) override;
		bool ClearGPUBufferComponent(GPUBufferComponent* rhs) override;

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
		bool Present() override;

		bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;

		Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) override;
		bool GenerateMipmap(TextureComponent* rhs) override;

		bool Resize() override;

		bool BeginCapture() override;
		bool EndCapture() override;
	};
}
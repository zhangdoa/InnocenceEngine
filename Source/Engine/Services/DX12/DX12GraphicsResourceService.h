#pragma once
#include "../GraphicsResourceService.h"

namespace Inno
{
	class IGraphicsService;

	class DX12GraphicsResourceService : public GraphicsResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12GraphicsResourceService);

		void SetBackend(IGraphicsService* backend) { m_Backend = backend; }

		MeshComponent* AddMeshComponent(const char* name) override;
		TextureComponent* AddTextureComponent(const char* name) override;
		MaterialComponent* AddMaterialComponent(const char* name) override;
		RenderPassComponent* AddRenderPassComponent(const char* name) override;
		ShaderProgramComponent* AddShaderProgramComponent(const char* name) override;
		SamplerComponent* AddSamplerComponent(const char* name) override;
		GPUBufferComponent* AddGPUBufferComponent(const char* name) override;
		CommandListComponent* AddCommandListComponent(const char* name) override;
		IPipelineStateObject* AddPipelineStateObject() override;
		ISemaphore* AddSemaphore() override;
		bool Add(IOutputMergerTarget*& rhs) override;

		bool Delete(MeshComponent* mesh) override;
		bool Delete(TextureComponent* texture) override;
		bool Delete(MaterialComponent* material) override;
		bool Delete(RenderPassComponent* renderPass) override;
		bool Delete(ShaderProgramComponent* shaderProgram) override;
		bool Delete(SamplerComponent* sampler) override;
		bool Delete(GPUBufferComponent* gpuBuffer) override;
		bool Delete(IPipelineStateObject* rhs) override;
		bool Delete(CommandListComponent* rhs) override;
		bool Delete(ISemaphore* rhs) override;
		bool Delete(IOutputMergerTarget* rhs) override;

		void Initialize(EntityID entity) override;
		void Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner) override;
		void Initialize(TextureComponent* texture, void* textureData, EntityID owner) override;
		void Initialize(MaterialComponent* material, EntityID owner) override;
		void Initialize(RenderPassComponent* renderPass) override;
		void Initialize(ShaderProgramComponent* shaderProgram) override;
		void Initialize(SamplerComponent* sampler) override;
		void Initialize(GPUBufferComponent* gpuBuffer) override;
		void Initialize(CommandListComponent* commandList) override;

		bool UploadToGPU(CommandListComponent* commandList, TextureComponent* texture) override;
		bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) override;
		bool Clear(CommandListComponent* commandList, TextureComponent* texture) override;
		bool Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) override;
		bool Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) override;
		bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList) override;

		TextureComponent* FindTextureByName(const char* name) override;
		MaterialComponent* FindMaterialByName(const char* name) override;
		std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) override;
		Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) override;

		GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle) override;
		GPUMeshResourceHandle FindMeshResourceByName(const char* name) override;

		GPUResourceComponent* GetTLASBuffer() override;
		bool OnSceneUnloading() override;

	protected:
		bool WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range) override;
		uint32_t GetCurrentFrame() override;

	private:
		IGraphicsService* m_Backend = nullptr;
	};
}

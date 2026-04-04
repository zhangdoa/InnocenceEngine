#pragma once
#include "../Interface/IService.h"
#include "../Common/GraphicsPrimitive.h"
#include "../Common/GPUDataStructure.h"
#include "../Common/Math.h"
#include "../Common/GPUMeshResource.h"
#include "../Component/GPUBufferComponent.h"

namespace Inno
{
	class MeshComponent;
	class TextureComponent;
	class MaterialComponent;
	class RenderPassComponent;
	class ShaderProgramComponent;
	class SamplerComponent;
	class GPUBufferComponent;
	class CommandListComponent;
	class GPUResourceComponent;

	class GraphicsResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(GraphicsResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override { return true; }
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override { return true; }
		ObjectStatus GetStatus() override { return ObjectStatus::Activated; }

		// Pool allocation
		virtual MeshComponent* AddMeshComponent(const char* name = "") = 0;
		virtual TextureComponent* AddTextureComponent(const char* name = "") = 0;
		virtual MaterialComponent* AddMaterialComponent(const char* name = "") = 0;
		virtual RenderPassComponent* AddRenderPassComponent(const char* name = "") = 0;
		virtual ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") = 0;
		virtual SamplerComponent* AddSamplerComponent(const char* name = "") = 0;
		virtual GPUBufferComponent* AddGPUBufferComponent(const char* name = "") = 0;
		virtual CommandListComponent* AddCommandListComponent(const char* name = "") = 0;
		virtual IPipelineStateObject* AddPipelineStateObject() = 0;
		virtual ISemaphore* AddSemaphore() = 0;
		virtual bool Add(IOutputMergerTarget*& rhs) = 0;

		// Pool deallocation
		virtual bool Delete(MeshComponent* mesh) = 0;
		virtual bool Delete(TextureComponent* texture) = 0;
		virtual bool Delete(MaterialComponent* material) = 0;
		virtual bool Delete(RenderPassComponent* renderPass) = 0;
		virtual bool Delete(ShaderProgramComponent* shaderProgram) = 0;
		virtual bool Delete(SamplerComponent* sampler) = 0;
		virtual bool Delete(GPUBufferComponent* gpuBuffer) = 0;
		virtual bool Delete(IPipelineStateObject* rhs) = 0;
		virtual bool Delete(CommandListComponent* rhs) = 0;
		virtual bool Delete(ISemaphore* rhs) = 0;
		virtual bool Delete(IOutputMergerTarget* rhs) = 0;

		// Initialization
		virtual void Initialize(EntityID entity) = 0;
		virtual void Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner = INVALID_ENTITY) = 0;
		virtual void Initialize(TextureComponent* texture, void* textureData = nullptr, EntityID owner = INVALID_ENTITY) = 0;
		virtual void Initialize(MaterialComponent* material, EntityID owner = INVALID_ENTITY) = 0;
		virtual void Initialize(RenderPassComponent* renderPass) = 0;
		virtual void Initialize(ShaderProgramComponent* shaderProgram) = 0;
		virtual void Initialize(SamplerComponent* sampler) = 0;
		virtual void Initialize(GPUBufferComponent* gpuBuffer) = 0;
		virtual void Initialize(CommandListComponent* commandList) = 0;

		// Upload / transfer
		virtual bool UploadToGPU(CommandListComponent* commandList, TextureComponent* texture) = 0;
		virtual bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) = 0;
		virtual bool Clear(CommandListComponent* commandList, TextureComponent* texture) = 0;
		virtual bool Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) = 0;
		virtual bool Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) = 0;
		virtual bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) = 0;

		// Query
		virtual TextureComponent* FindTextureByName(const char* name) = 0;
		virtual MaterialComponent* FindMaterialByName(const char* name) = 0;
		virtual std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) = 0;
		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) = 0;
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) = 0;

		// Mesh resource access
		virtual GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle) = 0;
		virtual GPUMeshResourceHandle FindMeshResourceByName(const char* name) = 0;

		// Upload helper
		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const T* value, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			auto l_mappedMemory = gpuBuffer->m_MappedMemories[GetCurrentFrame()];
			return WriteMappedMemory(gpuBuffer, l_mappedMemory, value, startOffset, range);
		}

		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const std::vector<T>& value, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return Upload(gpuBuffer, &value[0], startOffset, range);
		}

	protected:
		virtual bool WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range) = 0;
		virtual uint32_t GetCurrentFrame() = 0;
	};
}

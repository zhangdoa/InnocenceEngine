#pragma once
#include "../Interface/IService.h"
#include "../Common/GPUDataStructure.h"
#include "../Common/Math.h"
#include "../Common/GPUMeshResource.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Common/ThreadSafeVector.h"
#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/ObjectPool.h"
#include "../Common/EntityID.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/GPUBufferComponent.h"
#include "../Component/CommandListComponent.h"

namespace Inno
{
	class FrameManagementService;

	class GraphicsResourceService : public IService
	{
		friend class FrameManagementService;

	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(GraphicsResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		// Pool allocation (concrete - operates on owned pools)
		MeshComponent* AddMeshComponent(const char* name = "");
		TextureComponent* AddTextureComponent(const char* name = "");
		MaterialComponent* AddMaterialComponent(const char* name = "");
		RenderPassComponent* AddRenderPassComponent(const char* name = "");
		ShaderProgramComponent* AddShaderProgramComponent(const char* name = "");
		SamplerComponent* AddSamplerComponent(const char* name = "");
		GPUBufferComponent* AddGPUBufferComponent(const char* name = "");
		CommandListComponent* AddCommandListComponent(const char* name = "");

		// DX12-specific pool types (virtual - backend overrides)
		virtual IPipelineStateObject* AddPipelineStateObject() = 0;
		virtual ISemaphore* AddSemaphore() = 0;
		virtual bool Add(IOutputMergerTarget*& rhs) = 0;

		// Delete (virtual - backend overrides for DX12-specific resource cleanup)
		virtual bool Delete(MeshComponent* mesh);
		virtual bool Delete(TextureComponent* texture);
		virtual bool Delete(MaterialComponent* material);
		virtual bool Delete(RenderPassComponent* renderPass);
		virtual bool Delete(ShaderProgramComponent* shaderProgram);
		virtual bool Delete(SamplerComponent* sampler);
		virtual bool Delete(GPUBufferComponent* gpuBuffer);
		virtual bool Delete(IPipelineStateObject* rhs) = 0;
		virtual bool Delete(CommandListComponent* rhs);
		virtual bool Delete(ISemaphore* rhs) = 0;
		virtual bool Delete(IOutputMergerTarget* rhs) = 0;

		// Initialization (concrete - queues deferred work)
		void Initialize(EntityID entity);
		void Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner = INVALID_ENTITY);
		void Initialize(TextureComponent* texture, void* textureData = nullptr, EntityID owner = INVALID_ENTITY);
		void Initialize(MaterialComponent* material, EntityID owner = INVALID_ENTITY);
		void Initialize(RenderPassComponent* renderPass);
		void Initialize(ShaderProgramComponent* shaderProgram);
		void Initialize(SamplerComponent* sampler);
		void Initialize(GPUBufferComponent* gpuBuffer);
		void Initialize(CommandListComponent* commandList);

		// Upload / transfer (virtual - backend overrides)
		virtual bool UploadToGPU(CommandListComponent* commandList, TextureComponent* texture) { return false; }
		virtual bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return false; }
		virtual bool Clear(CommandListComponent* commandList, TextureComponent* texture) { return false; }
		virtual bool Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return false; }
		virtual bool Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) { return false; }
		virtual bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) { return false; }

		// Query (virtual - backend overrides)
		virtual std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) { return std::nullopt; }
		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) { return Vec4(); }
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) { return std::vector<Vec4>(); }

		// Lookup
		TextureComponent* FindTextureByName(const char* name);
		MaterialComponent* FindMaterialByName(const char* name);

		// Mesh resource access
		GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle);
		const GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle) const;
		GPUMeshResourceHandle FindMeshResourceByName(const char* name);

		// Raytracing
		GPUResourceComponent* GetTLASBuffer();

		// Scene lifecycle
		bool OnSceneUnloading();

		// Process deferred initialization queues (called by FrameManagementService)
		bool InitializeComponents();

		// Render pass lifecycle
		bool InitializeRenderPass(RenderPassComponent* renderPass);
		bool CreateOutputMergerTargets(RenderPassComponent* renderPass);
		bool InitializeOutputMergerTargets(RenderPassComponent* renderPass);
		bool DeleteRenderTargets(RenderPassComponent* renderPass);

		// Upload helper
		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const T* value, size_t startOffset = 0, size_t range = SIZE_MAX);

		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const std::vector<T>& value, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return Upload(gpuBuffer, &value[0], startOffset, range);
		}

		bool WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range);

		// Accessors for cross-service use
		ThreadSafeVector<GPUBufferComponent*>& GetGPUBufferPointers() { return m_GPUHandlePools.GPUBufferPointers; }
		ThreadSafeVector<RenderPassComponent*>& GetRenderPassPointers() { return m_GPUHandlePools.RenderPassPointers; }
		bool IsTLASReady() const { return m_TLASReady; }
		void SetTLASReady(bool ready) { m_TLASReady = ready; }

		// Cross-service accessors for FrameManagementService
		void ForEachCommandList(std::function<void(CommandListComponent*)> func) { m_GPUHandlePools.CommandListPointers.for_each(func); }
		GPUBufferComponent* GetTLASBufferComponent() { return m_TLASBufferComponent; }
		GPUBufferComponent* GetScratchBufferComponent() { return m_ScratchBufferComponent; }
		GPUBufferComponent* GetRaytracingInstanceBufferComponent() { return m_RaytracingInstanceBufferComponent; }
		std::vector<IRaytracingInstanceDescList*>& GetRaytracingInstanceDescs() { return m_RaytracingInstanceDescs; }

		template <typename T>
		void ReleaseFromPool(TObjectPool<T>* pool,
		                     ThreadSafeUnorderedMap<std::string, T*>& lut,
		                     ThreadSafeVector<T*>& pointers,
		                     T* ptr)
		{
			if (!ptr) return;
			lut.erase(std::string(ptr->m_InstanceName.c_str()));
			pointers.eraseByValue(ptr);
			pool->Destroy(ptr);
		}

		uint32_t GetCurrentFrameIndex();

	protected:
		// DX12-specific initialization (virtual - backend overrides)
		virtual bool InitializeImpl(MeshAssetHandle handle, std::vector<Vertex>& vertices, std::vector<Index>& indices) { return false; }
		virtual void ReleaseMeshGPUResourceImpl(MeshAssetHandle handle) = 0;
		virtual bool InitializeImpl(TextureComponent* texture, void* textureData) { return false; }
		virtual bool InitializeImpl(MaterialComponent* material);
		virtual bool InitializeImpl(ShaderProgramComponent* shaderProgram) { return false; }
		virtual bool InitializeImpl(SamplerComponent* sampler) { return false; }
		virtual bool InitializeImpl(GPUBufferComponent* gpuBuffer) { return false; }
		virtual bool InitializeImpl(EntityID entity) { return false; }
		virtual bool InitializeImpl(CommandListComponent* commandList) { return false; }

		// Render pass initialization helpers (virtual - backend overrides)
		virtual bool OnOutputMergerTargetsCreated(RenderPassComponent* renderPass) { return false; }
		virtual bool CreatePipelineStateObject(RenderPassComponent* renderPass) { return false; }
		virtual bool CreateFenceEvents(RenderPassComponent* renderPass) { return false; }
		virtual bool OnSceneLoadingStart() { return false; }

		// Pool lifecycle (virtual - backend may add DX12-specific pools)
		virtual bool InitializePool() { return true; }
		virtual bool TerminatePool() { return true; }

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

		// GPU handle pools (moved from IGraphicsService)
		struct GPUHandlePools
		{
			TObjectPool<MeshComponent>*          Meshes          = nullptr;
			TObjectPool<TextureComponent>*       Textures        = nullptr;
			TObjectPool<MaterialComponent>*      Materials       = nullptr;
			TObjectPool<RenderPassComponent>*    RenderPasses    = nullptr;
			TObjectPool<ShaderProgramComponent>* ShaderPrograms  = nullptr;
			TObjectPool<SamplerComponent>*       Samplers        = nullptr;
			TObjectPool<GPUBufferComponent>*     GPUBuffers      = nullptr;
			TObjectPool<CommandListComponent>*   CommandLists    = nullptr;

			ThreadSafeUnorderedMap<std::string, MeshComponent*>          MeshLUT;
			ThreadSafeUnorderedMap<std::string, TextureComponent*>       TextureLUT;
			ThreadSafeUnorderedMap<std::string, MaterialComponent*>      MaterialLUT;
			ThreadSafeUnorderedMap<std::string, RenderPassComponent*>    RenderPassLUT;
			ThreadSafeUnorderedMap<std::string, ShaderProgramComponent*> ShaderProgramLUT;
			ThreadSafeUnorderedMap<std::string, SamplerComponent*>       SamplerLUT;
			ThreadSafeUnorderedMap<std::string, GPUBufferComponent*>     GPUBufferLUT;
			ThreadSafeUnorderedMap<std::string, CommandListComponent*>   CommandListLUT;

			ThreadSafeVector<MeshComponent*>          MeshPointers;
			ThreadSafeVector<TextureComponent*>       TexturePointers;
			ThreadSafeVector<MaterialComponent*>      MaterialPointers;
			ThreadSafeVector<RenderPassComponent*>    RenderPassPointers;
			ThreadSafeVector<ShaderProgramComponent*> ShaderProgramPointers;
			ThreadSafeVector<SamplerComponent*>       SamplerPointers;
			ThreadSafeVector<GPUBufferComponent*>     GPUBufferPointers;
			ThreadSafeVector<CommandListComponent*>   CommandListPointers;
		};
		GPUHandlePools m_GPUHandlePools;

		std::vector<GPUMeshResource> m_MeshResources;
		std::vector<uint32_t> m_FreeMeshResourceSlots;
		ThreadSafeUnorderedMap<std::string, GPUMeshResourceHandle> m_MeshResourceLUT;

		std::unordered_set<TextureComponent*>  m_initializedTextures;
		std::unordered_set<MaterialComponent*> m_initializedMaterials;
		std::unordered_set<EntityID>           m_initializedEntities;

		GPUBufferComponent* m_TLASBufferComponent = nullptr;
		GPUBufferComponent* m_ScratchBufferComponent = nullptr;
		GPUBufferComponent* m_RaytracingInstanceBufferComponent = nullptr;
		bool m_TLASReady = false;
		std::vector<IRaytracingInstanceDescList*> m_RaytracingInstanceDescs;

	private:
		GPUMeshResourceHandle AllocateMeshResource(const char* name, ObjectLifespan lifespan);
		void ReleaseMeshResource(GPUMeshResourceHandle handle);
		void ReleaseAllMeshResources(ObjectLifespan lifespan);

		struct MeshInitTask
		{
			MeshInitTask(MeshComponent* component, std::vector<Vertex>&& vertices, std::vector<Index>&& indices, EntityID owner = INVALID_ENTITY, ObjectLifespan lifespan = ObjectLifespan::Invalid)
				: m_Component(component), m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_Owner(owner), m_Lifespan(lifespan) {}

			MeshComponent* m_Component;
			std::vector<Vertex> m_Vertices;
			std::vector<Index> m_Indices;
			EntityID m_Owner;
			ObjectLifespan m_Lifespan;
		};

		struct TextureInitTask
		{
			TextureInitTask(TextureComponent* component, void* textureData, EntityID owner = INVALID_ENTITY)
				: m_Component(component), m_TextureData(textureData), m_Owner(owner) {}

			TextureComponent* m_Component;
			void* m_TextureData;
			EntityID m_Owner;
		};

		struct MaterialInitTask
		{
			MaterialInitTask(MaterialComponent* component, EntityID owner = INVALID_ENTITY)
				: m_Component(component), m_Owner(owner) {}

			MaterialComponent* m_Component;
			EntityID m_Owner;
		};

		ThreadSafeQueue<MeshInitTask> m_uninitializedMeshes;
		ThreadSafeQueue<TextureInitTask> m_uninitializedTextures;
		ThreadSafeQueue<MaterialInitTask> m_uninitializedMaterials;
		ThreadSafeQueue<GPUBufferComponent*> m_uninitializedGPUBuffers;
		ThreadSafeQueue<RenderPassComponent*> m_uninitializedRenderPasses;
		ThreadSafeQueue<EntityID> m_uninitializedEntities;
	};

	template<typename T>
	bool GraphicsResourceService::Upload(GPUBufferComponent* gpuBuffer, const T* value, size_t startOffset, size_t range)
	{
		auto l_currentFrame = GetCurrentFrameIndex();
		auto l_mappedMemory = gpuBuffer->m_MappedMemories[l_currentFrame];
		return WriteMappedMemory(gpuBuffer, l_mappedMemory, value, startOffset, range);
	}
}

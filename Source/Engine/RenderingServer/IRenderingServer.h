#pragma once
#include "../Interface/IService.h"

#include "../Common/ThreadSafeQueue.h"
#include "../Common/ObjectPool.h"
#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/ThreadSafeVector.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/GPUBufferComponent.h"
#include "../Component/CommandListComponent.h"
#include "../Common/EntityID.h"

namespace Inno
{
	class IRenderingServer : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingServer);

		bool Setup(IServiceConfig* systemConfig) override;
        bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		// Component Pool APIs
		virtual MeshComponent* AddMeshComponent(const char* name = "");
		virtual TextureComponent* AddTextureComponent(const char* name = "");
		virtual MaterialComponent* AddMaterialComponent(const char* name = "");
		virtual RenderPassComponent* AddRenderPassComponent(const char* name = "");
		virtual ShaderProgramComponent* AddShaderProgramComponent(const char* name = "");
		virtual SamplerComponent* AddSamplerComponent(const char* name = "");
		virtual GPUBufferComponent* AddGPUBufferComponent(const char* name = "");
		virtual CommandListComponent* AddCommandListComponent(const char* name = "");
		virtual IPipelineStateObject* AddPipelineStateObject() = 0;

		TextureComponent*  FindTextureByName(const char* name);
		MeshComponent*     FindMeshByName(const char* name);
		MaterialComponent* FindMaterialByName(const char* name);
		virtual ISemaphore* AddSemaphore() = 0;
		virtual bool Add(IOutputMergerTarget*& rhs) = 0;

		virtual	bool Delete(MeshComponent* mesh) = 0;
		virtual	bool Delete(TextureComponent* texture) = 0;
		virtual	bool Delete(MaterialComponent* material) = 0;
		virtual	bool Delete(RenderPassComponent* renderPass) = 0;
		virtual	bool Delete(ShaderProgramComponent* shaderProgram) = 0;
		virtual	bool Delete(SamplerComponent* sampler) = 0;
		virtual	bool Delete(GPUBufferComponent* gpuBuffer) = 0;
		virtual	bool Delete(IPipelineStateObject* rhs) = 0;
		virtual	bool Delete(CommandListComponent* rhs) = 0;
		virtual	bool Delete(ISemaphore* rhs) = 0;
		virtual	bool Delete(IOutputMergerTarget* rhs) = 0;

		void Initialize(EntityID Entity);
		void Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices);
		void Initialize(TextureComponent* texture, void* textureData = nullptr);
		void Initialize(MaterialComponent* material);
		void Initialize(RenderPassComponent* renderPass);
		void Initialize(ShaderProgramComponent* shaderProgram);
		void Initialize(SamplerComponent* sampler);
		void Initialize(GPUBufferComponent* gpuBuffer);
		void Initialize(CommandListComponent* commandList);

		// Rendering Stage APIs
		void SetUploadHeapPreparationCallback(std::function<bool()>&& callback);
		void SetCommandPreparationCallback(std::function<bool()>&& callback);
		void SetCommandExecutionCallback(std::function<bool()>&& callback);

		uint32_t GetSwapChainImageCount();
		RenderPassComponent* GetSwapChainRenderPassComponent();
		uint32_t GetPreviousFrame();
		uint32_t GetCurrentFrame();
		uint32_t GetNextFrame();
		uint32_t GetFrameCountSinceLaunch();

		// Command List APIs
		virtual bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex);
		virtual bool BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList) { return false; }
		virtual bool ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index = -1) { return false; }
		virtual bool BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual bool TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) { return false; }
		virtual bool TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) { return false; }

		virtual bool ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand) { return false; }
		virtual void PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants) {}
		virtual bool DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount = 1) { return false; }
		virtual bool DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount = 1) { return false; }
		
		virtual bool SignalOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType);
		virtual bool WaitOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType, GPUEngineType semaphoreType);
		virtual bool CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList);
		virtual bool UnbindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual bool Present();
		virtual bool Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) { return false; }

		virtual bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) { return false; }
		virtual bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) { return false; }
		virtual bool Execute(CommandListComponent* commandList, GPUEngineType queueType) { return false; }
		virtual uint64_t GetSemaphoreValue(GPUEngineType queueType) { return 0; }
		virtual bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) { return false; }

		virtual bool SetUserPipelineOutput(std::function<GPUResourceComponent* ()>&& getUserPipelineOutputFunc);
		virtual GPUResourceComponent* GetUserPipelineOutput();

		virtual bool Resize();

		virtual bool UploadToGPU(CommandListComponent* commandList, MeshComponent* mesh) { return false; }
		virtual bool UploadToGPU(CommandListComponent* commandList, TextureComponent* texture) { return false; }
		virtual bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return false; }
		virtual bool Clear(CommandListComponent* commandList, TextureComponent* texture) { return false; }
		virtual bool Copy(CommandListComponent* commandList, TextureComponent* sourceTexture, TextureComponent* destinationTexture) { return false; }
		virtual bool Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return false; }

		virtual bool Open(CommandListComponent* commandList, GPUEngineType GPUEngineType, IPipelineStateObject* pipelineStateObject = nullptr) { return false; }
		virtual bool Close(CommandListComponent* commandList, GPUEngineType GPUEngineType) { return false; }

		// Component APIs
		virtual std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) { return std::nullopt; }
		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) { return Vec4(); }
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) { return std::vector<Vec4>(); }
		virtual bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) { return false; }

		// Debug use only
		virtual bool BeginCapture() { return false; }
		virtual bool EndCapture() { return false; }
		virtual bool HasGPUError() const { return false; }

		// Raytracing-related APIs
		virtual bool DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ) { return false; }
		GPUResourceComponent* GetTLASBuffer() { return m_TLASBufferComponent; }

	protected:
		bool WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range);

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

	public:
		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const T* GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			auto l_mappedMemory = gpuBuffer->m_MappedMemories[GetCurrentFrame()];
			return WriteMappedMemory(gpuBuffer, l_mappedMemory, GPUBufferValue, startOffset, range);
		}

		template<typename T>
		bool Upload(GPUBufferComponent* gpuBuffer, const std::vector<T>& GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return Upload(gpuBuffer, &GPUBufferValue[0], startOffset, range);
		}

	protected:
		virtual bool InitializeImpl(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices) { return false; }
		virtual bool InitializeImpl(TextureComponent* texture, void* textureData) { return false; }
		virtual bool InitializeImpl(MaterialComponent* material);
		virtual bool InitializeImpl(RenderPassComponent* renderPass);
		virtual	bool InitializeImpl(ShaderProgramComponent* shaderProgram) { return false; }
		virtual bool InitializeImpl(SamplerComponent* sampler) { return false; }
		virtual bool InitializeImpl(GPUBufferComponent* gpuBuffer) { return false; }
		virtual bool InitializeImpl(EntityID Entity) { return false; }
		virtual bool InitializeImpl(CommandListComponent* commandList) { return false; }

        bool DeleteRenderTargets(RenderPassComponent* renderPass);

		virtual bool ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility);

		virtual bool InitializePool();
		virtual bool TerminatePool();

		virtual bool CreateHardwareResources() = 0;
		virtual bool ReleaseHardwareResources() = 0;
		virtual bool GetSwapChainImages() = 0;
		virtual bool AssignSwapChainImages() = 0;
		virtual bool ReleaseSwapChainImages() = 0;
        bool InitializeSwapChainRenderPassComponent();
		
		bool CreateOutputMergerTargets(RenderPassComponent* renderPass);
		bool InitializeOutputMergerTargets(RenderPassComponent* renderPass);
        virtual bool OnOutputMergerTargetsCreated(RenderPassComponent* renderPass) { return false; }
		virtual bool CreatePipelineStateObject(RenderPassComponent* renderPass) { return false; }
		virtual bool CreateFenceEvents(RenderPassComponent* renderPass) { return false; }

		virtual bool BeginFrame() { return false; }
		virtual bool PrepareRayTracing(CommandListComponent* commandList) { return false; }
		virtual bool PresentImpl() { return false; }
		virtual bool WaitAllOnCPU() { return false; }
		virtual bool ResizeImpl() { return false; }
		virtual bool EndFrame() { return false; }

		virtual bool OnSceneLoadingStart() { return false; }

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);

		std::vector<CommandListComponent*> m_GlobalGraphicsCommandLists;
		ISemaphore* m_GlobalSemaphore;

		uint32_t m_swapChainImageCount = 2;
		uint32_t m_PreviousFrame = 1;
		uint32_t m_CurrentFrame = 0;
		std::atomic<uint32_t> m_FrameCountSinceLaunch = 0;

		std::vector<uint64_t> m_GraphicsSemaphoreValues;
		std::vector<uint64_t> m_ComputeSemaphoreValues;
		std::vector<uint64_t> m_CopySemaphoreValues;

		std::function<GPUResourceComponent* ()> m_GetUserPipelineOutputFunc;
		RenderPassComponent* m_SwapChainRenderPassComp;
		ShaderProgramComponent* m_SwapChainShaderProgramComp;
		SamplerComponent* m_SwapChainSamplerComp;

		std::function<bool()> m_UploadHeapPreparationCallback;
		std::function<bool()> m_CommandPreparationCallback;
		std::function<bool()> m_CommandExecutionCallback;
		
		std::function<void()> m_SceneLoadingStartedCallback;

		// Init task objects for deferred component initialization
		struct MeshInitTask
		{
			MeshInitTask(MeshComponent* component, std::vector<Vertex>&& vertices, std::vector<Index>&& indices)
				: m_Component(component), m_Vertices(std::move(vertices)), m_Indices(std::move(indices)) {}
			
			MeshComponent* m_Component;
			std::vector<Vertex> m_Vertices;
			std::vector<Index> m_Indices;
		};

		struct TextureInitTask
		{
			TextureInitTask(TextureComponent* component, void* textureData)
				: m_Component(component), m_TextureData(textureData) {}
			
			TextureComponent* m_Component;
			void* m_TextureData;
		};

		ThreadSafeQueue<MeshInitTask> m_uninitializedMeshes;
		ThreadSafeQueue<TextureInitTask> m_uninitializedTextures;
		ThreadSafeQueue<MaterialComponent*> m_uninitializedMaterials;
		ThreadSafeQueue<GPUBufferComponent*> m_uninitializedGPUBuffers;
		ThreadSafeQueue<RenderPassComponent*> m_uninitializedRenderPasses;
		ThreadSafeQueue<EntityID> m_uninitializedEntities;

		std::unordered_set<MeshComponent*> m_initializedMeshes;
		std::unordered_set<TextureComponent*> m_initializedTextures;
		std::unordered_set<MaterialComponent*> m_initializedMaterials;
		std::unordered_set<GPUBufferComponent*> m_initializedGPUBuffers;
		std::vector<RenderPassComponent*> m_initializedRenderPasses;
		std::unordered_set<EntityID> m_initializedEntities;
	
        GPUBufferComponent* m_TLASBufferComponent = nullptr;
		GPUBufferComponent* m_ScratchBufferComponent = nullptr;
 		GPUBufferComponent* m_RaytracingInstanceBufferComponent = nullptr;

        std::vector<IRaytracingInstanceDescList*> m_RaytracingInstanceDescs;

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

	private:
		bool InitializeComponents();
		bool PrepareGlobalCommands();
		bool ExecuteGlobalCommands();
		bool PrepareSwapChainCommands();
		bool ExecuteSwapChainCommands();

		bool ExecuteResize();
		bool PreResize();
    	bool PreResize(RenderPassComponent* renderPass);
		bool PostResize();
		bool PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* renderPass);

		std::atomic_bool m_needResize = false;
	};
}
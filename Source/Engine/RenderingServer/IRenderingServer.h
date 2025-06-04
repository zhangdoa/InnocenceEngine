#pragma once
#include "../Interface/ISystem.h"

#include "../Common/ThreadSafeQueue.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/GPUBufferComponent.h"
#include "../Component/CollisionComponent.h"

namespace Inno
{
	class IRenderingServer : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingServer);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		// Component Pool APIs
		virtual MeshComponent* AddMeshComponent(const char* name = "") = 0;
		virtual TextureComponent* AddTextureComponent(const char* name = "") = 0;
		virtual MaterialComponent* AddMaterialComponent(const char* name = "") = 0;
		virtual RenderPassComponent* AddRenderPassComponent(const char* name = "") = 0;
		virtual ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") = 0;
		virtual SamplerComponent* AddSamplerComponent(const char* name = "") = 0;
		virtual GPUBufferComponent* AddGPUBufferComponent(const char* name = "") = 0;
		virtual IPipelineStateObject* AddPipelineStateObject() = 0;
		virtual ICommandList* AddCommandList() = 0;
		virtual ISemaphore* AddSemaphore() = 0;
		virtual bool Add(IOutputMergerTarget*& rhs) = 0;

		virtual	bool Delete(MeshComponent* rhs) = 0;
		virtual	bool Delete(TextureComponent* rhs) = 0;
		virtual	bool Delete(MaterialComponent* rhs) = 0;
		virtual	bool Delete(RenderPassComponent* rhs) = 0;
		virtual	bool Delete(ShaderProgramComponent* rhs) = 0;
		virtual	bool Delete(SamplerComponent* rhs) = 0;
		virtual	bool Delete(GPUBufferComponent* rhs) = 0;
		virtual	bool Delete(IPipelineStateObject* rhs) = 0;
		virtual	bool Delete(ICommandList* rhs) = 0;
		virtual	bool Delete(ISemaphore* rhs) = 0;
		virtual	bool Delete(IOutputMergerTarget* rhs) = 0;

		void Initialize(MeshComponent* rhs);
		void Initialize(TextureComponent* rhs);
		void Initialize(MaterialComponent* rhs);
		void Initialize(RenderPassComponent* rhs);
		void Initialize(ShaderProgramComponent* rhs);
		void Initialize(SamplerComponent* rhs);
		void Initialize(GPUBufferComponent* rhs);

		void Initialize(CollisionComponent* rhs);

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
		virtual bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex);
		virtual bool BindRenderPassComponent(RenderPassComponent* rhs) { return false; }
		virtual bool ClearRenderTargets(RenderPassComponent* rhs, size_t index = -1) { return false; }
		virtual bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual bool TryToTransitState(TextureComponent* rhs, ICommandList* commandList, Accessibility accessibility) { return false; }

		virtual bool UpdateIndirectDrawCommand(GPUBufferComponent* indirectDrawCommand, const std::vector<DrawCallInfo>& drawCallList, std::function<bool(const DrawCallInfo&)>&& isDrawCallValid) { return false; }
		virtual bool ExecuteIndirect(RenderPassComponent* rhs, GPUBufferComponent* indirectDrawCommand) { return false; }
		virtual void PushRootConstants(RenderPassComponent* rhs, size_t rootConstants) {}
		virtual bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount = 1) { return false; }
		virtual bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount = 1) { return false; }
		
		virtual bool SignalOnGPU(RenderPassComponent* rhs, GPUEngineType queueType);
		virtual bool WaitOnGPU(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType);
		virtual bool CommandListEnd(RenderPassComponent* rhs);
		virtual bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType);
		virtual bool Present();
		virtual bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) { return false; }

		virtual bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) { return false; }
		virtual bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) { return false; }
		virtual bool Execute(ICommandList* commandList, GPUEngineType queueType) { return false; }
		virtual uint64_t GetSemaphoreValue(GPUEngineType queueType) { return 0; }
		virtual bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) { return false; }

		virtual bool SetUserPipelineOutput(std::function<GPUResourceComponent* ()>&& getUserPipelineOutputFunc);
		virtual GPUResourceComponent* GetUserPipelineOutput();

		virtual bool Resize();

		// Component APIs
		virtual bool Clear(TextureComponent* rhs);
		virtual bool Copy(TextureComponent* lhs, TextureComponent* rhs);
		virtual bool Clear(GPUBufferComponent* rhs);
		virtual std::optional<uint32_t> GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility) { return std::nullopt; }
		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) { return Vec4(); }
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) { return std::vector<Vec4>(); }
		virtual bool GenerateMipmap(TextureComponent* rhs, ICommandList* commandList = nullptr) { return false; }

		// Debug use only
		virtual bool BeginCapture() { return false; }
		virtual bool EndCapture() { return false; }

		// Raytracing-related APIs
		virtual bool DispatchRays(RenderPassComponent* rhs, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ) { return false; }
		GPUResourceComponent* GetTLASBuffer() { return m_TLASBufferComponent; }

	protected:
		bool WriteMappedMemory(MeshComponent* rhs);
		bool WriteMappedMemory(GPUBufferComponent* rhs, IMappedMemory* mappedMemory, const void* GPUBufferValue, size_t startOffset, size_t range);

	public:
		template<typename T>
		bool Upload(GPUBufferComponent* rhs, const T* GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			auto l_mappedMemory = rhs->m_MappedMemories[GetCurrentFrame()];
			return WriteMappedMemory(rhs, l_mappedMemory, GPUBufferValue, startOffset, range);
		}

		template<typename T>
		bool Upload(GPUBufferComponent* rhs, const std::vector<T>& GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return Upload(rhs, &GPUBufferValue[0], startOffset, range);
		}

	protected:
		virtual bool InitializeImpl(MeshComponent* rhs) { return false; }
		virtual bool InitializeImpl(TextureComponent* rhs) { return false; }
		virtual bool InitializeImpl(MaterialComponent* rhs);
		virtual bool InitializeImpl(RenderPassComponent* rhs);
		virtual	bool InitializeImpl(ShaderProgramComponent* rhs) { return false; }
		virtual bool InitializeImpl(SamplerComponent* rhs) { return false; }
		virtual bool InitializeImpl(GPUBufferComponent* rhs) { return false; }

		virtual bool InitializeImpl(CollisionComponent* rhs) { return false; }

        bool DeleteRenderTargets(RenderPassComponent* rhs);

		virtual bool UploadToGPU(ICommandList* commandList, MeshComponent* rhs) { return false; }
		virtual bool UploadToGPU(ICommandList* commandList, TextureComponent* rhs) { return false; }
		virtual bool UploadToGPU(ICommandList* commandList, GPUBufferComponent* rhs) { return false; }
		virtual bool Clear(ICommandList* commandList, TextureComponent* rhs) { return false; }
		virtual bool Copy(ICommandList* commandList, TextureComponent* lhs, TextureComponent* rhs) { return false; }
		virtual bool Clear(ICommandList* commandList, GPUBufferComponent* rhs) { return false; }

		virtual bool Open(ICommandList* commandList, GPUEngineType GPUEngineType, IPipelineStateObject* pipelineStateObject = nullptr) { return false; }
		virtual bool Close(ICommandList* commandList, GPUEngineType GPUEngineType) { return false; }

		virtual bool ChangeRenderTargetStates(RenderPassComponent* renderPass, ICommandList* commandList, Accessibility accessibility);

		virtual bool InitializePool() { return false; }
		virtual bool TerminatePool() { return false; }

		virtual bool CreateHardwareResources() = 0;
		virtual bool ReleaseHardwareResources() = 0;
		virtual bool GetSwapChainImages() = 0;
		virtual bool AssignSwapChainImages() = 0;
		virtual bool ReleaseSwapChainImages() = 0;

		bool CreateOutputMergerTargets(RenderPassComponent* rhs);
		bool InitializeOutputMergerTargets(RenderPassComponent* rhs);
        virtual bool OnOutputMergerTargetsCreated(RenderPassComponent* rhs) { return false; }
		virtual bool CreatePipelineStateObject(RenderPassComponent* rhs) { return false; }
		virtual bool CreateCommandList(ICommandList* commandList, size_t swapChainImageIndex, const std::wstring& name) { return false; }
		virtual bool CreateFenceEvents(RenderPassComponent* rhs) { return false; }

		virtual bool BeginFrame() { return false; }
		virtual bool PrepareRayTracing(ICommandList* commandList) { return false; }
		virtual bool PresentImpl() { return false; }
		virtual bool WaitAllOnCPU() { return false; }
		virtual bool ResizeImpl() { return false; }
		virtual bool EndFrame() { return false; }

		virtual bool OnSceneLoadingStart() { return false; }

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);

		std::vector<ICommandList*> m_GlobalCommandLists;
		ISemaphore* m_GlobalSemaphore;

		uint32_t m_swapChainImageCount = 2;
		uint32_t m_PreviousFrame = 1;
		uint32_t m_CurrentFrame = 0;
		std::atomic<uint32_t> m_FrameCountSinceLaunch = 0;
		
		std::vector<uint64_t> m_CopyUploadToDefaultHeapSemaphoreValues;
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

		ThreadSafeQueue<MeshComponent*> m_uninitializedMeshes;
		ThreadSafeQueue<TextureComponent*> m_uninitializedTextures;
		ThreadSafeQueue<MaterialComponent*> m_uninitializedMaterials;
		ThreadSafeQueue<GPUBufferComponent*> m_uninitializedGPUBuffers;
		ThreadSafeQueue<RenderPassComponent*> m_uninitializedRenderPasses;

		struct CopyCommand
		{
			CopyCommand(TextureComponent* lhs, TextureComponent* rhs) : m_lhs(lhs), m_rhs(rhs) {}
			TextureComponent* m_lhs;
			TextureComponent* m_rhs;
		};

		ThreadSafeQueue<CopyCommand> m_unprocessedCopyCommands;
		ThreadSafeQueue<GPUResourceComponent*> m_unclearedResources;

		std::unordered_set<MeshComponent*> m_initializedMeshes;
		std::unordered_set<TextureComponent*> m_initializedTextures;
		std::unordered_set<MaterialComponent*> m_initializedMaterials;
		std::unordered_set<GPUBufferComponent*> m_initializedGPUBuffers;
		std::vector<RenderPassComponent*> m_initializedRenderPasses;
	
        GPUBufferComponent* m_TLASBufferComponent = nullptr;
		GPUBufferComponent* m_ScratchBufferComponent = nullptr;
 		GPUBufferComponent* m_RaytracingInstanceBufferComponent = nullptr;
		std::shared_mutex m_CollisionComponentsMutex;
		std::unordered_map<MeshComponent*, std::vector<CollisionComponent*>> m_UnregisteredCollisionComponents;
        std::unordered_set<CollisionComponent*> m_RegisteredCollisionComponents;
        std::vector<IRaytracingInstanceDescList*> m_RaytracingInstanceDescs;

	private:
		bool InitializeComponents();
		bool PrepareGlobalCommands();
		bool ExecuteGlobalCommands();
		bool PrepareSwapChainCommands();
		bool ExecuteSwapChainCommands();

		bool ExecuteResize();
		bool PreResize();
    	bool PreResize(RenderPassComponent* rhs);
		bool PostResize();
		bool PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* rhs);

		std::atomic_bool m_needResize = false;
	};
}
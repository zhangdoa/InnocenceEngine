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

		// Component-related APIs
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

		virtual	bool DeleteMeshComponent(MeshComponent* rhs) = 0;
		virtual	bool DeleteTextureComponent(TextureComponent* rhs) = 0;
		virtual	bool DeleteMaterialComponent(MaterialComponent* rhs) = 0;
		virtual	bool DeleteRenderPassComponent(RenderPassComponent* rhs) = 0;
		virtual	bool DeleteShaderProgramComponent(ShaderProgramComponent* rhs) = 0;
		virtual	bool DeleteSamplerComponent(SamplerComponent* rhs) = 0;
		virtual	bool DeleteGPUBufferComponent(GPUBufferComponent* rhs) = 0;
		virtual	bool Delete(IPipelineStateObject* rhs) = 0;
		virtual	bool Delete(ICommandList* rhs) = 0;
		virtual	bool Delete(ISemaphore* rhs) = 0;

		void InitializeMeshComponent(MeshComponent* rhs);
		void InitializeTextureComponent(TextureComponent* rhs);
		void InitializeMaterialComponent(MaterialComponent* rhs);
		void InitializeRenderPassComponent(RenderPassComponent *rhs);
		void InitializeShaderProgramComponent(ShaderProgramComponent* rhs);
		void InitializeSamplerComponent(SamplerComponent* rhs);
		void InitializeGPUBufferComponent(GPUBufferComponent* rhs);

		virtual uint32_t GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility) { return 0; }

		bool ReserveRenderTargets(RenderPassComponent* rhs);
        bool CreateRenderTargets(RenderPassComponent* rhs);
		virtual bool CreatePipelineStateObject(RenderPassComponent* rhs) {	return false; }
        virtual bool CreateCommandList(ICommandList* commandList, size_t swapChainImageIndex, const std::wstring& name) {	return false; }
        virtual bool CreateFenceEvents(RenderPassComponent* rhs) {	return false; }

		// Rendering APIs
		virtual void ExecuteGlobalCommands();
		uint32_t GetSwapChainImageCount();
		RenderPassComponent* GetSwapChainRenderPassComponent();
		uint32_t GetPreviousFrame();
		uint32_t GetCurrentFrame();
		virtual bool PrepareSwapChainCommands();
		virtual bool ExecuteSwapChainCommands();
		
		virtual bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex);
		virtual bool BindRenderPassComponent(RenderPassComponent* rhs) { return false; }
		virtual bool ClearRenderTargets(RenderPassComponent* rhs, size_t index = -1) { return false; }
		virtual bool Bind(RenderPassComponent* renderPass, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc) { return false; }
		virtual bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual void PushRootConstants(RenderPassComponent* rhs, size_t rootConstants) {}
		virtual bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount = 1) { return false; }
		virtual bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount = 1) { return false; }
		virtual bool SignalOnGPU(RenderPassComponent* rhs, GPUEngineType queueType);
		virtual bool WaitOnGPU(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType);
		virtual bool CommandListEnd(RenderPassComponent* rhs);
		virtual bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) { return false; }
		virtual bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType);
		virtual bool Present();
		virtual bool PostPresent() { return false; }

		virtual bool TryToTransitState(TextureComponent *rhs, ICommandList *commandList, Accessibility accessibility) { return false; }

		virtual bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc);
		virtual GPUResourceComponent* GetUserPipelineOutput();
		
		virtual bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) { return false; }

		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) { return Vec4(); }
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) { return std::vector<Vec4>(); }
		virtual bool GenerateMipmap(TextureComponent* rhs) { return false; }

		virtual bool Resize();

		// Debug use only
		virtual bool BeginCapture() { return false; }
		virtual bool EndCapture() { return false; }

	protected:
		bool WriteMappedMemory(MeshComponent* rhs);
		bool WriteMappedMemory(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range);

	public:
		virtual bool ClearTextureComponent(TextureComponent* rhs) { return false; }
		virtual bool CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs) { return false; }
		virtual bool ClearGPUBufferComponent(GPUBufferComponent* rhs) { return false; }

		template<typename T>
		bool UploadGPUBufferComponent(GPUBufferComponent* rhs, const T* GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return WriteMappedMemory(rhs, GPUBufferValue, startOffset, range);
		}

		template<typename T>
		bool UploadGPUBufferComponent(GPUBufferComponent* rhs, const std::vector<T>& GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return UploadGPUBufferComponent(rhs, &GPUBufferValue[0], startOffset, range);
		}

		virtual bool SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType) { return false; }
		virtual bool WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType) { return false; }
		virtual bool Execute(ICommandList* commandList, GPUEngineType queueType) { return false; }
        virtual uint64_t GetSemaphoreValue(GPUEngineType queueType) { return 0; }
		virtual bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) { return false; }

	protected:
		virtual bool InitializeImpl(MeshComponent* rhs) { return false; }
		virtual bool InitializeImpl(TextureComponent* rhs) { return false; }
		virtual bool InitializeImpl(MaterialComponent* rhs);
		virtual bool InitializeImpl(RenderPassComponent* rhs);
		virtual	bool InitializeImpl(ShaderProgramComponent* rhs) { return false; }
		virtual bool InitializeImpl(SamplerComponent* rhs) { return false; }
		virtual bool InitializeImpl(GPUBufferComponent* rhs) { return false; }

		virtual bool UploadToGPU(ICommandList* commandList, MeshComponent* rhs) { return false; }
		virtual bool UploadToGPU(ICommandList* commandList, TextureComponent* rhs) { return false; }
		virtual bool UploadToGPU(ICommandList* commandList, GPUBufferComponent* rhs) { return false; }
		virtual bool Open(ICommandList* commandList, GPUEngineType GPUEngineType, IPipelineStateObject* pipelineStateObject = nullptr) { return false; }
		virtual bool Close(ICommandList* commandList, GPUEngineType GPUEngineType) { return false; }

		virtual bool ChangeRenderTargetStates(RenderPassComponent* renderPass, ICommandList* commandList, Accessibility accessibility);
		
		virtual bool InitializePool() { return false; }
		virtual bool TerminatePool() { return false; }

		virtual bool CreateHardwareResources() = 0;
        virtual bool ReleaseHardwareResources() = 0;
		virtual bool GetSwapChainImages() = 0;
		virtual bool AssignSwapChainImages() = 0;

		virtual bool PresentImpl() { return false; }

		virtual bool ResizeImpl() { return false; }

		virtual bool PreResize(RenderPassComponent* rhs);
		virtual bool PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* rhs) { return false; };

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
        TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);
		
		std::vector<ICommandList*> m_GlobalCommandLists;
		ISemaphore* m_GlobalSemaphore;

		const uint32_t m_swapChainImageCount = 2;
		uint32_t m_PreviousFrame = 1;
		uint32_t m_CurrentFrame	= 0;

        std::function<GPUResourceComponent* ()> m_GetUserPipelineOutputFunc;
        RenderPassComponent* m_SwapChainRenderPassComp;
        ShaderProgramComponent* m_SwapChainSPC;
        SamplerComponent* m_SwapChainSamplerComp;

		ThreadSafeQueue<MeshComponent*> m_uninitializedMeshes;
		ThreadSafeQueue<TextureComponent*> m_uninitializedTextures;
		ThreadSafeQueue<MaterialComponent*> m_uninitializedMaterials;
		ThreadSafeQueue<GPUBufferComponent*> m_uninitializedGPUBuffers;
		ThreadSafeQueue<RenderPassComponent*> m_uninitializedRenderPasses;

        std::unordered_set<MeshComponent*> m_initializedMeshes;
        std::unordered_set<TextureComponent*> m_initializedTextures;
        std::unordered_set<MaterialComponent*> m_initializedMaterials;
		std::unordered_set<GPUBufferComponent*> m_initializedGPUBuffers;
        std::vector<RenderPassComponent*> m_initializedRenderPasses;
		
	private:
		bool ExecuteResize();
		bool PreResize();
		bool PostResize();

		std::atomic_bool m_needResize = false;
	};
}
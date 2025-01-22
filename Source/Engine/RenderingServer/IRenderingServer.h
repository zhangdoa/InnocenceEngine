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
	class IRenderingServer;
	class IRenderingComponentPool;
	class IRenderingGraphicsDeviceSystemConfig : public ISystemConfig
	{
	public:
		IRenderingServer* m_RenderingServer;
		IRenderingComponentPool* m_RenderingComponentPool;
	};

	class IRenderingGraphicsDevice : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingGraphicsDevice);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		bool OnFrameEnd() override;

		virtual ObjectStatus GetStatus() = 0;

		virtual bool CreateHardwareResources() = 0;
        virtual bool ReleaseHardwareResources() = 0;
		virtual bool GetSwapChainImages() = 0;
		virtual bool AssignSwapChainImages() = 0;
		
		uint32_t GetSwapChainImageCount();
		RenderPassComponent* GetSwapChainRenderPassComponent();
		uint32_t GetCurrentFrame() { return m_SwapChainRenderPassComp->m_CurrentFrame; }
		virtual bool FinalizeSwapChain();
		virtual bool Present();
		virtual bool PresentImpl() {}
		virtual bool PostPresent() {}
		virtual bool Resize();

		// Debug use only
		virtual bool BeginCapture();
		virtual bool EndCapture();

	protected:
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		IRenderingServer* m_RenderingServer;
		IRenderingComponentPool* m_RenderingComponentPool;
		
        TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);
		
		const uint32_t m_swapChainImageCount = 2;		
        std::function<GPUResourceComponent* ()> m_GetUserPipelineOutputFunc;
        RenderPassComponent* m_SwapChainRenderPassComp;
        ShaderProgramComponent* m_SwapChainSPC;
        SamplerComponent* m_SwapChainSamplerComp;
	};

	class IRenderingComponentPool
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingComponentPool);

		virtual void Initialize() = 0;
		virtual void Terminate() = 0;

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

        bool ReserveRenderTargets(RenderPassComponent* rhs);

		void InitializeMeshComponent(MeshComponent* rhs);
		void InitializeMaterialComponent(MaterialComponent* rhs);
		void InitializeGPUBufferComponent(GPUBufferComponent* rhs);
		void InitializeRenderPassComponent(RenderPassComponent *rhs);

		virtual void TransferDataToGPU();

		bool PreResize();
		bool PreResize(RenderPassComponent* rhs);
		bool PostResize();

	protected:
		ThreadSafeQueue<MeshComponent*> m_uninitializedMeshes;
		ThreadSafeQueue<MaterialComponent*> m_uninitializedMaterials;

        std::unordered_set<MeshComponent*> m_initializedMeshes;
        std::unordered_set<TextureComponent*> m_initializedTextures;
        std::unordered_set<MaterialComponent*> m_initializedMaterials;
        std::vector<RenderPassComponent*> m_initializedRenderPasses;
		
        std::vector<GPUBufferComponent*> m_uninitializedBuffers;
	};

	class IRenderingServer : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingServer);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		IRenderingGraphicsDevice* GetRenderingGraphicsDevice() { return m_Device; }
		IRenderingComponentPool* GetRenderingComponentPool() { return m_RenderingComponentPool; }

		virtual bool InitializeMeshComponent(MeshComponent* rhs);
		virtual bool InitializeTextureComponent(TextureComponent* rhs);
		virtual bool InitializeMaterialComponent(MaterialComponent* rhs);
		virtual bool InitializeRenderPassComponent(RenderPassComponent* rhs);
		virtual	bool InitializeShaderProgramComponent(ShaderProgramComponent* rhs);
		virtual bool InitializeSamplerComponent(SamplerComponent* rhs);
		virtual bool InitializeGPUBufferComponent(GPUBufferComponent* rhs);

		virtual bool UpdateMeshComponent(MeshComponent* rhs);
		virtual bool UpdateGPUBufferComponent(GPUBufferComponent* rhs);

		virtual bool ClearTextureComponent(TextureComponent* rhs);
		virtual bool CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs);
		
		virtual uint32_t GetIndex(GPUResourceComponent* rhs) { return 0; }
		
        bool CreateRenderTargets(RenderPassComponent* rhs);

		virtual void PushRootConstants(RenderPassComponent* rhs, size_t rootConstants) {}
		virtual bool Bind(RenderPassComponent* renderPass, GPUResourceComponent* resource, ShaderStage shaderStage, uint32_t rootParameterIndex, Accessibility bindingAccessibility) { return false; }
		virtual bool TryToTransitState(TextureComponent *rhs, ICommandList *commandList, Accessibility accessibility);

	protected:
		virtual IRenderingGraphicsDevice* CreateRenderingGraphicsDevice() { return nullptr; }
		virtual IRenderingComponentPool* CreateRenderingComponentPool() { return nullptr; }

		virtual bool UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range);

	public:
		template<typename T>
		bool UploadGPUBufferComponent(GPUBufferComponent* rhs, const T* GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return UploadGPUBufferComponentImpl(rhs, GPUBufferValue, startOffset, range);
		}

		template<typename T>
		bool UploadGPUBufferComponent(GPUBufferComponent* rhs, const std::vector<T>& GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return UploadGPUBufferComponentImpl(rhs, &GPUBufferValue[0], startOffset, range);
		}

		virtual bool ClearGPUBufferComponent(GPUBufferComponent* rhs);

		virtual bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex);
		virtual bool BindRenderPassComponent(RenderPassComponent* rhs);
		virtual bool ClearRenderTargets(RenderPassComponent* rhs, size_t index = -1);
		virtual bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount = SIZE_MAX);
		virtual bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount = 1);
		virtual bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount = 1);
		virtual bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount = SIZE_MAX);
		virtual bool CommandListEnd(RenderPassComponent* rhs);
		virtual bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType);
		virtual bool WaitCommandQueue(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType);
		virtual bool WaitFence(RenderPassComponent* rhs, GPUEngineType GPUEngineType);
		virtual bool Present();
		
		virtual bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc);
		virtual GPUResourceComponent* GetUserPipelineOutput();
		
		virtual bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ);

		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y);
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp);
		virtual bool GenerateMipmap(TextureComponent* rhs);

		virtual bool Resize();
		
	protected:
		bool ResizeImpl();
		std::atomic_bool m_needResize = false;

		IRenderingGraphicsDevice* m_Device = nullptr;
		IRenderingComponentPool* m_RenderingComponentPool = nullptr;
	};
}
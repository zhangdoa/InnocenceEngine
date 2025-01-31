#pragma once
#include "../IRenderingServer.h"
#include "DX12Headers.h"

#include "../../Common/ObjectPool.h"

#include "../../Component/DX12MeshComponent.h"
#include "../../Component/DX12TextureComponent.h"
#include "../../Component/DX12MaterialComponent.h"
#include "../../Component/DX12RenderPassComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../../Component/DX12SamplerComponent.h"
#include "../../Component/DX12GPUBufferComponent.h"

namespace Inno
{
    class DX12RenderingServer : public IRenderingServer
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(DX12RenderingServer);

        bool Update() override;
        
        // Inherited via IRenderingServer
        // In DX12RenderingServer_ComponentPool.cpp
        MeshComponent* AddMeshComponent(const char* name = "") override;
        TextureComponent* AddTextureComponent(const char* name = "") override;
        MaterialComponent* AddMaterialComponent(const char* name = "") override;
        RenderPassComponent* AddRenderPassComponent(const char* name = "") override;
        ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") override;
        SamplerComponent* AddSamplerComponent(const char* name = "") override;
        GPUBufferComponent* AddGPUBufferComponent(const char* name = "") override;
        IPipelineStateObject* AddPipelineStateObject() override;
        ICommandList* AddCommandList() override;
        ISemaphore* AddSemaphore() override;

        virtual	bool DeleteMeshComponent(MeshComponent* rhs) override;
        virtual	bool DeleteTextureComponent(TextureComponent* rhs) override;
        virtual	bool DeleteMaterialComponent(MaterialComponent* rhs) override;
        virtual	bool DeleteRenderPassComponent(RenderPassComponent* rhs) override;
        virtual	bool DeleteShaderProgramComponent(ShaderProgramComponent* rhs) override;
        virtual	bool DeleteSamplerComponent(SamplerComponent* rhs) override;
        virtual	bool DeleteGPUBufferComponent(GPUBufferComponent* rhs) override;
        virtual	bool Delete(IPipelineStateObject* rhs) override;
        virtual	bool Delete(ICommandList* rhs) override;
        virtual	bool Delete(ISemaphore* rhs) override;

        // In DX12RenderingServer.cpp
        bool ClearTextureComponent(TextureComponent* rhs) override;
        bool CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs) override;
        bool ClearGPUBufferComponent(GPUBufferComponent* rhs) override;

        uint32_t GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility) override;

        bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex) override;
        bool BindRenderPassComponent(RenderPassComponent* rhs) override;
        bool ClearRenderTargets(RenderPassComponent* rhs, size_t index = -1) override;
        bool Bind(RenderPassComponent* renderPass, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc) override;
        bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount) override;
        void PushRootConstants(RenderPassComponent* rhs, size_t rootConstants) override;
        bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount) override;
        bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount) override;
        bool CommandListEnd(RenderPassComponent* rhs) override;
        bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType) override;
        bool WaitCommandQueue(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType) override;
        bool WaitFence(RenderPassComponent* rhs, GPUEngineType GPUEngineType) override;

        bool TryToTransitState(TextureComponent* rhs, ICommandList* commandList, Accessibility accessibility) override;

        bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;

        Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) override;
        std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) override;
        bool GenerateMipmap(TextureComponent* rhs) override;

        // In DX12RenderingServer_GraphicsDevice_Protected.cpp
        bool BeginCapture() override;
        bool EndCapture() override;

        // Getters
        ComPtr<ID3D12Device8> GetDevice();
        ComPtr<ID3D12CommandAllocator> GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
        ComPtr<ID3D12CommandQueue> GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
        ComPtr<ID3D12GraphicsCommandList> GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE commandListType);
        DX12DescriptorHeapAccessor& GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility = Accessibility::ReadOnly
        , Accessibility resourceAccessibility = Accessibility::ReadOnly, TextureUsage textureUsage = TextureUsage::Invalid, bool isShaderVisible = true);

    protected:
        // In DX12RenderingServer_ComponentPool.cpp
        bool InitializeImpl(MeshComponent* rhs) override;
        bool InitializeImpl(TextureComponent* rhs) override;
        bool InitializeImpl(ShaderProgramComponent* rhs) override;
        bool InitializeImpl(SamplerComponent* rhs) override;
        bool InitializeImpl(GPUBufferComponent* rhs) override;

        bool UploadToGPU(ICommandList* commandList, MeshComponent* rhs) override;
        bool UploadToGPU(ICommandList* commandList, TextureComponent* rhs) override;
        bool UploadToGPU(ICommandList* commandList, GPUBufferComponent* rhs) override;

        bool Open(ICommandList* commandList, GPUEngineType GPUEngineType, IPipelineStateObject* pipelineStateObject) override;
        bool Close(ICommandList* commandList, GPUEngineType GPUEngineType) override;
        bool Execute(ICommandList* commandList, ISemaphore* semaphore, GPUEngineType GPUEngineType) override;
        bool Wait(ISemaphore* rhs, GPUEngineType queueType, GPUEngineType semaphoreType) override;

        bool InitializePool() override;
        bool TerminatePool() override;
        
        bool CreatePipelineStateObject(RenderPassComponent* rhs) override;
        bool CreateCommandList(ICommandList* commandList, size_t swapChainImageIndex, const std::wstring& name) override;
        bool CreateFenceEvents(RenderPassComponent* rhs) override;

        // In DX12RenderingServer_GraphicsDevice_Protected.cpp
        bool CreateHardwareResources() override;
        bool ReleaseHardwareResources() override;
        bool GetSwapChainImages() override;
        bool AssignSwapChainImages() override;

        bool PresentImpl() override;
        bool PostPresent() override;

        bool ResizeImpl() override;
        bool PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* rhs) override;

    private:
        // Global initialization functions
        // In DX12RenderingServer_GraphicsDevice_Private.cpp
        bool CreateDebugCallback();
        bool CreatePhysicalDevices();
        bool CreateGlobalCommandQueues();
        bool CreateGlobalCommandAllocators();
        bool CreateSyncPrimitives();
        bool CreateGlobalDescriptorHeaps();
        bool CreateMipmapGenerator();
        bool CreateSwapChain();

        // APIs for DX12 objects
        // In DX12RenderingServer_DX12Object.cpp
        ComPtr<ID3D12Resource> CreateUploadHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, const char* name = "");
        ComPtr<ID3D12Resource> CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, D3D12_CLEAR_VALUE* clearValue = nullptr, const char* name = "");
        ComPtr<ID3D12Resource> CreateReadBackHeapBuffer(UINT64 size, const char* name = "");
        ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_QUEUE_DESC* commandQueueDesc, const wchar_t* name = L"");
        ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, const wchar_t* name = L"");
        ComPtr<ID3D12GraphicsCommandList> CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator, const wchar_t* name = L"");
        ComPtr<ID3D12GraphicsCommandList> CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator);
        ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc, const wchar_t* name = L"");

        bool ExecuteCommandListAndWait(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandQueue> commandQueue);

        // APIs for engine components
        // In DX12RenderingServer_EngineComponent_Private.cpp
        DX12DescriptorHeapAccessor CreateDescriptorHeapAccessor
        (
            ComPtr<ID3D12DescriptorHeap> descHeap, 
            D3D12_DESCRIPTOR_HEAP_DESC desc, 
            uint32_t maxDescriptors, 
            uint32_t descriptorSize, 
            const DX12DescriptorHandle& firstHandle,
            bool shaderVisible,
            const wchar_t* name
        );
        bool CreateRTV(DX12RenderPassComponent* rhs);
        bool CreateDSV(DX12RenderPassComponent* rhs);
        DX12SRV CreateSRV(DX12TextureComponent* rhs, uint32_t mostDetailedMip);
        DX12SRV CreateSRV(DX12GPUBufferComponent* rhs);
        DX12UAV CreateUAV(DX12TextureComponent* rhs, uint32_t mipSlice);
        DX12UAV CreateUAV(DX12GPUBufferComponent* rhs);
        DX12CBV CreateCBV(DX12GPUBufferComponent* rhs);
        
        bool CreateRootSignature(DX12RenderPassComponent* DX12RenderPassComp);
        D3D12_DESCRIPTOR_RANGE1 GetDescriptorRange(DX12RenderPassComponent* DX12RenderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc);

        bool BindComputeResource(DX12CommandList* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource);
        bool BindGraphicsResource(DX12CommandList* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource);
        bool SetDescriptorHeaps(DX12RenderPassComponent* renderPass, DX12CommandList* commandList);
        bool SetRenderTargets(DX12RenderPassComponent* renderPass, DX12CommandList* commandList);
        bool PreparePipeline(DX12RenderPassComponent* renderPass, DX12CommandList* commandList, DX12PipelineStateObject* PSO);
        bool WaitFence(DX12Semaphore* rhs, GPUEngineType GPUEngineType);
        bool TryToTransitState(DX12TextureComponent* rhs, DX12CommandList* commandList, const D3D12_RESOURCE_STATES& newState);

        bool GenerateMipmapImpl(DX12TextureComponent* DX12TextureComp);

        // DX12 objects
        int32_t m_videoCardMemory = 0;
        char m_videoCardDescription[128];

        ComPtr<ID3D12Debug1> m_debugInterface = nullptr;
        ComPtr<IDXGraphicsAnalysis> m_graphicsAnalysis = nullptr;

        ComPtr<IDXGIFactory7> m_factory = nullptr;

        DXGI_ADAPTER_DESC m_adapterDesc = {};
        ComPtr<IDXGIAdapter4> m_adapter = nullptr;
        ComPtr<IDXGIOutput6> m_adapterOutput = nullptr;

        ComPtr<ID3D12Device8> m_device = nullptr;

        DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc = {};
        ComPtr<IDXGISwapChain4> m_swapChain = nullptr;
        std::vector<ComPtr<ID3D12Resource>> m_swapChainImages;

        ComPtr<ID3D12CommandQueue> m_directCommandQueue = nullptr;
        ComPtr<ID3D12CommandQueue> m_computeCommandQueue = nullptr;
        ComPtr<ID3D12CommandQueue> m_copyCommandQueue = nullptr;

        ComPtr<ID3D12Fence> m_directCommandQueueFence = nullptr;
        ComPtr<ID3D12Fence> m_computeCommandQueueFence = nullptr;
        ComPtr<ID3D12Fence> m_copyCommandQueueFence = nullptr;
        std::vector<ComPtr<ID3D12CommandAllocator>> m_directCommandAllocators;
        std::vector<ComPtr<ID3D12CommandAllocator>> m_computeCommandAllocators;
        std::vector<ComPtr<ID3D12CommandAllocator>> m_copyCommandAllocators;

        ComPtr<ID3D12DescriptorHeap> m_CSUDescHeap = nullptr;
        DX12DescriptorHeapAccessor m_GPUBuffer_CBV_DescHeapAccessor;

        DX12DescriptorHeapAccessor m_GPUBuffer_SRV_DescHeapAccessor;

        DX12DescriptorHeapAccessor m_MaterialTexture_SRV_DescHeapAccessor; // @TODO: This is only for read-only material textures
        DX12DescriptorHeapAccessor m_RenderTarget_SRV_DescHeapAccessor; // @TODO: Change this to for render targets since they can't be together with the read-only material textures

        DX12DescriptorHeapAccessor m_GPUBuffer_UAV_DescHeapAccessor;
        DX12DescriptorHeapAccessor m_MaterialTexture_UAV_DescHeapAccessor;
        DX12DescriptorHeapAccessor m_RenderTarget_UAV_DescHeapAccessor;

        ComPtr<ID3D12DescriptorHeap> m_CSUDescHeap_ShaderNonVisible = nullptr;
        DX12DescriptorHeapAccessor m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible;
        DX12DescriptorHeapAccessor m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible;
        DX12DescriptorHeapAccessor m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible;

        ComPtr<ID3D12DescriptorHeap> m_RTVDescHeap = nullptr;
        DX12DescriptorHeapAccessor m_RTVDescHeapAccessor;

        ComPtr<ID3D12DescriptorHeap> m_DSVDescHeap = nullptr;
        DX12DescriptorHeapAccessor m_DSVDescHeapAccessor;

        ComPtr<ID3D12DescriptorHeap> m_SamplerDescHeap = nullptr;
        DX12DescriptorHeapAccessor m_SamplerDescHeapAccessor;
        
        ID3D12RootSignature* m_2DMipmapRootSignature = nullptr;
        ID3D12RootSignature* m_3DMipmapRootSignature = nullptr;
        ID3D12PipelineState* m_2DMipmapPSO = nullptr;
        ID3D12PipelineState* m_3DMipmapPSO = nullptr;

        // Component pools
        TObjectPool<DX12MeshComponent>* m_MeshComponentPool = nullptr;
        TObjectPool<DX12MaterialComponent>* m_MaterialComponentPool = nullptr;
        TObjectPool<DX12TextureComponent>* m_TextureComponentPool = nullptr;
        TObjectPool<DX12RenderPassComponent>* m_RenderPassComponentPool = nullptr;
        TObjectPool<DX12ShaderProgramComponent>* m_ShaderProgramComponentPool = nullptr;
        TObjectPool<DX12SamplerComponent>* m_SamplerComponentPool = nullptr;
        TObjectPool<DX12GPUBufferComponent>* m_GPUBufferComponentPool = nullptr;
        TObjectPool<DX12PipelineStateObject>* m_PSOPool = nullptr;
        TObjectPool<DX12CommandList>* m_CommandListPool = nullptr;
        TObjectPool<DX12Semaphore>* m_SemaphorePool = nullptr;
    };
}
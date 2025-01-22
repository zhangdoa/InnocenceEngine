#pragma once
#include "../IRenderingServer.h"

#include "../../Common/ObjectPool.h"

#include "DX12Headers.h"
#include "../../Component/DX12MeshComponent.h"
#include "../../Component/DX12TextureComponent.h"
#include "../../Component/DX12MaterialComponent.h"
#include "../../Component/DX12RenderPassComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../../Component/DX12SamplerComponent.h"
#include "../../Component/DX12GPUBufferComponent.h"

namespace Inno
{
    class DX12GraphicsDevice : public IRenderingGraphicsDevice
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(DX12GraphicsDevice);

        // Inherited via IRenderingGraphicsDevice
        bool Setup(ISystemConfig* systemConfig = nullptr) override;
        bool Initialize() override;
        bool Update() override;
        bool OnFrameEnd() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        bool CreateHardwareResources() override;
        bool ReleaseHardwareResources() override;
        bool GetSwapChainImages() override;
        bool AssignSwapChainImages() override;

        bool PresentImpl() override;
        bool PostPresent() override;
        bool Resize() override;

        bool BeginCapture() override;
        bool EndCapture() override;

        // Getters
        ComPtr<ID3D12Device8> GetDevice();
        ComPtr<ID3D12CommandAllocator> GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
        ComPtr<ID3D12CommandQueue> GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
        ComPtr<ID3D12GraphicsCommandList> GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE commandListType);

        // Global initialization functions
        bool CreateDebugCallback();
        bool CreatePhysicalDevices();
        bool CreateGlobalCommandQueues();
        bool CreateGlobalCommandAllocators();
        bool CreateSyncPrimitives();
        bool CreateGlobalDescriptorHeaps();
        bool CreateMipmapGenerator();
        bool CreateSwapChain();

        // APIs for DX12 objects
        DX12SRV CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC desc, ComPtr<ID3D12Resource> resourceHandle);
        DX12UAV CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC desc, ComPtr<ID3D12Resource> resourceHandle, bool isAtomicCounter);

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
        bool CreateRTV(DX12RenderPassComponent* rhs);
        bool CreateDSV(DX12RenderPassComponent* rhs);
        DX12SRV CreateSRV(DX12TextureComponent* rhs, uint32_t mostDetailedMip);
        DX12SRV CreateSRV(DX12GPUBufferComponent* rhs);
        DX12UAV CreateUAV(DX12TextureComponent* rhs, uint32_t mipSlice);
        DX12UAV CreateUAV(DX12GPUBufferComponent* rhs);
        DX12CBV CreateCBV(DX12GPUBufferComponent* rhs);

        bool CreateRootSignature(DX12RenderPassComponent* DX12RenderPassComp);
        bool CreatePSO(DX12RenderPassComponent* DX12RenderPassComp);
        bool CreateCommandList(DX12CommandList* commandList, size_t swapChainImageIndex, const std::wstring& name);
        bool CreateFenceEvents(DX12RenderPassComponent *DX12RenderPassComp);

        DX12DescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, bool isShaderVisible = true);

        bool PrepareRenderTargets(DX12RenderPassComponent* renderPass, DX12CommandList* commandList);
        bool SetDescriptorHeaps(DX12RenderPassComponent* renderPass, DX12CommandList* commandList);
        bool SetRenderTargets(DX12RenderPassComponent* renderPass, DX12CommandList* commandList);
        bool PreparePipeline(DX12RenderPassComponent* renderPass, DX12CommandList* commandList, DX12PipelineStateObject* PSO);
        bool ExecuteCommandList(DX12CommandList* commandList, DX12Semaphore* semaphore, GPUEngineType GPUEngineType);
        bool WaitCommandQueue(DX12Semaphore* rhs, GPUEngineType queueType, GPUEngineType semaphoreType);
        bool WaitFence(DX12Semaphore* rhs, GPUEngineType GPUEngineType);

        bool PostResize(DX12RenderPassComponent* rhs, const TVec2<uint32_t>& screenResolution);

        bool GenerateMipmap(DX12TextureComponent* DX12TextureComp);
        bool TryToTransitState(DX12TextureComponent *rhs, DX12CommandList *commandList, const D3D12_RESOURCE_STATES& newState);

    private:
        int32_t m_videoCardMemory = 0;
        char m_videoCardDescription[128];

        ComPtr<ID3D12Debug1> m_debugInterface = 0;
        ComPtr<IDXGraphicsAnalysis> m_graphicsAnalysis = 0;

        ComPtr<IDXGIFactory7> m_factory = 0;

        DXGI_ADAPTER_DESC m_adapterDesc = {};
        ComPtr<IDXGIAdapter4> m_adapter = 0;
        ComPtr<IDXGIOutput6> m_adapterOutput = 0;

        ComPtr<ID3D12Device8> m_device = 0;

        DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc = {};
        ComPtr<IDXGISwapChain4> m_swapChain = 0;
        std::vector<ComPtr<ID3D12Resource>> m_swapChainImages;

        ComPtr<ID3D12CommandQueue> m_directCommandQueue = 0;
        ComPtr<ID3D12CommandQueue> m_computeCommandQueue = 0;
        ComPtr<ID3D12CommandQueue> m_copyCommandQueue = 0;

        std::vector<std::atomic<uint64_t>> m_directCommandQueueSemaphore;
        std::vector<std::atomic<uint64_t>> m_computeCommandQueueSemaphore;
        std::vector<std::atomic<uint64_t>> m_copyCommandQueueSemaphore;
        std::vector<ComPtr<ID3D12Fence>> m_directCommandQueueFence;
        std::vector<ComPtr<ID3D12Fence>> m_computeCommandQueueFence;
        std::vector<ComPtr<ID3D12Fence>> m_copyCommandQueueFence;
        std::vector<std::atomic<HANDLE>> m_directCommandQueueFenceEvent;
        std::vector<std::atomic<HANDLE>> m_computeCommandQueueFenceEvent;
        std::vector<ComPtr<ID3D12CommandAllocator>> m_directCommandAllocators;
        std::vector<ComPtr<ID3D12CommandAllocator>> m_computeCommandAllocators;
        ComPtr<ID3D12CommandAllocator> m_copyCommandAllocator = 0;
        std::vector<DX12CommandList*> m_GlobalCommandLists;

        DX12DescriptorHeap* m_CSUDescHeap = 0;
        DX12DescriptorHeap* m_ShaderNonVisibleCSUDescHeap = 0;
        DX12DescriptorHeap* m_RTVDescHeap = 0;
        DX12DescriptorHeap* m_DSVDescHeap = 0;
        DX12DescriptorHeap* m_SamplerDescHeap = 0;

        ID3D12RootSignature* m_2DMipmapRootSignature = 0;
        ID3D12RootSignature* m_3DMipmapRootSignature = 0;
        ID3D12PipelineState* m_2DMipmapPSO = 0;
        ID3D12PipelineState* m_3DMipmapPSO = 0;
    };
}
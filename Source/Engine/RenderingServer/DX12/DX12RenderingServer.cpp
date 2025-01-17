#include "DX12RenderingServer.h"

#include "../../Component/DX12MeshComponent.h"
#include "../../Component/DX12TextureComponent.h"
#include "../../Component/DX12MaterialComponent.h"
#include "../../Component/DX12RenderPassComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../../Component/DX12SamplerComponent.h"
#include "../../Component/DX12GPUBufferComponent.h"

#include "../../Platform/WinWindow/WinWindowSystem.h"

#include <DXProgrammableCapture.h>
#include <DXGIDebug.h>

#ifdef max
#undef max
#endif

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Engine.h"

using namespace Inno;

#include "../Common/Helper.h"
using namespace RenderingServerHelper;

#include "DX12Helper.h"
using namespace DX12Helper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/EntityManager.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace DX12RenderingServerNS
{
	bool CreateDebugCallback();
	bool CreatePhysicalDevices();
	bool CreateGlobalCommandQueues();
	bool CreateGlobalCommandAllocators();
	bool CreateSyncPrimitives();
	bool CreateGlobalCSUDescHeap();
	bool CreateGlobalRTVDescHeap();
	bool CreateGlobalDSVDescHeap();
	bool CreateGlobalSamplerDescHeap();
	bool CreateMipmapGenerator();
	bool CreateSwapChain();
	bool GetSwapChainImages();
	bool AssignSwapChainImages();

	DX12PipelineStateObject *AddPSO();
	DX12CommandList *AddCommandList();
	DX12Semaphore *AddSemaphore();

	void CreateCommandList(DX12CommandList* commandList, size_t swapChainImageIndex, const std::wstring& name);
	void CreateCommandLists(DX12RenderPassComponent *DX12RenderPassComp);
	bool GenerateMipmapImpl(DX12TextureComponent *DX12TextureComp);
	bool DeleteResources(DX12RenderPassComponent* rhs, DX12RenderingServer* renderingServer);
	bool Resize(const TVec2<uint32_t>& screenResolution, DX12RenderPassComponent* rhs, DX12RenderingServer* renderingServer);
	bool ResizeImpl();
	
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	TObjectPool<DX12MeshComponent> *m_MeshComponentPool = 0;
	TObjectPool<DX12MaterialComponent> *m_MaterialComponentPool = 0;
	TObjectPool<DX12TextureComponent> *m_TextureComponentPool = 0;
	TObjectPool<DX12RenderPassComponent> *m_RenderPassComponentPool = 0;
	TObjectPool<DX12PipelineStateObject> *m_PSOPool = 0;
	TObjectPool<DX12CommandList> *m_CommandListPool = 0;
	TObjectPool<DX12Semaphore> *m_SemaphorePool = 0;
	TObjectPool<DX12ShaderProgramComponent> *m_ShaderProgramComponentPool = 0;
	TObjectPool<DX12SamplerComponent> *m_SamplerComponentPool = 0;
	TObjectPool<DX12GPUBufferComponent> *m_GPUBufferComponentPool = 0;

	std::unordered_set<MeshComponent*> m_initializedMeshes;
	std::unordered_set<TextureComponent*> m_initializedTextures;
	std::unordered_set<MaterialComponent*> m_initializedMaterials;
	std::vector<RenderPassComponent*> m_initializedRenderPasses;
	std::vector<GPUBufferComponent*> m_dirtyBuffers;

	std::atomic_bool m_needResize = false;

	TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);

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
	const uint32_t m_swapChainImageCount = 2;
	std::vector<ComPtr<ID3D12Resource>> m_swapChainImages;

	ComPtr<ID3D12CommandQueue> m_directCommandQueue = 0;
	ComPtr<ID3D12CommandQueue> m_computeCommandQueue = 0;
	ComPtr<ID3D12CommandQueue> m_copyCommandQueue = 0;

	std::vector<std::atomic<uint64_t>> m_directCommandQueueSemaphore(m_swapChainImageCount);
	std::vector<std::atomic<uint64_t>> m_computeCommandQueueSemaphore(m_swapChainImageCount);
	std::vector<std::atomic<uint64_t>> m_copyCommandQueueSemaphore(m_swapChainImageCount);
	std::vector<ComPtr<ID3D12Fence>> m_directCommandQueueFence(m_swapChainImageCount);
	std::vector<ComPtr<ID3D12Fence>> m_computeCommandQueueFence(m_swapChainImageCount);
	std::vector<ComPtr<ID3D12Fence>> m_copyCommandQueueFence(m_swapChainImageCount);
	std::vector<std::atomic<HANDLE>> m_directCommandQueueFenceEvent(m_swapChainImageCount);
	std::vector<std::atomic<HANDLE>> m_computeCommandQueueFenceEvent(m_swapChainImageCount);
	std::vector<ComPtr<ID3D12CommandAllocator>> m_directCommandAllocators(m_swapChainImageCount);
	std::vector<ComPtr<ID3D12CommandAllocator>> m_computeCommandAllocators(m_swapChainImageCount);
	ComPtr<ID3D12CommandAllocator> m_copyCommandAllocator = 0;
	std::vector<DX12CommandList*> m_GlobalCommandLists(m_swapChainImageCount);
	
	ComPtr<ID3D12DescriptorHeap> m_CSUDescHeap = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CSUDescHeapCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_CSUDescHeapGPUHandle;

	ComPtr<ID3D12DescriptorHeap> m_ShaderNonVisibleCSUDescHeap = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE m_ShaderNonVisibleCSUDescHeapCPUHandle;

	ComPtr<ID3D12DescriptorHeap> m_RTVDescHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC m_RTVDescHeapDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVDescHeapCPUHandle;

	ComPtr<ID3D12DescriptorHeap> m_DSVDescHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC m_DSVDescHeapDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVDescHeapCPUHandle;

	ComPtr<ID3D12DescriptorHeap> m_samplerDescHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC m_samplerDescHeapDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_SamplerDescHeapCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_SamplerDescHeapGPUHandle;

	std::function<GPUResourceComponent*()> m_GetUserPipelineOutputFunc;
	DX12RenderPassComponent *m_SwapChainRenderPassComp = 0;
	DX12ShaderProgramComponent *m_SwapChainSPC = 0;
	DX12SamplerComponent *m_SwapChainSamplerComp = 0;

	ID3D12RootSignature *m_2DMipmapRootSignature = 0;
	ID3D12RootSignature *m_3DMipmapRootSignature = 0;
	ID3D12PipelineState *m_2DMipmapPSO = 0;
	ID3D12PipelineState *m_3DMipmapPSO = 0;
} // namespace DX12RenderingServerNS

DX12PipelineStateObject* DX12RenderingServerNS::AddPSO()
{
	return m_PSOPool->Spawn();
}

DX12CommandList* DX12RenderingServerNS::AddCommandList()
{
	return m_CommandListPool->Spawn();
}

DX12Semaphore* DX12RenderingServerNS::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

void DX12RenderingServerNS::CreateCommandList(DX12CommandList* commandList, size_t swapChainImageIndex, const std::wstring& name)
{
	commandList->m_DirectCommandList = DX12Helper::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_device, m_directCommandAllocators[swapChainImageIndex].Get(), (name + L"_DirectCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	commandList->m_ComputeCommandList = DX12Helper::CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_device, m_computeCommandAllocators[swapChainImageIndex].Get(), (name + L"_ComputeCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	commandList->m_CopyCommandList = DX12Helper::CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_device, m_copyCommandAllocator, (name + L"_CopyCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());

	commandList->m_DirectCommandList->Close();
	commandList->m_ComputeCommandList->Close();
	commandList->m_CopyCommandList->Close();
}

void DX12RenderingServerNS::CreateCommandLists(DX12RenderPassComponent *DX12RenderPassComp)
{
	auto l_tempName = std::string(DX12RenderPassComp->m_InstanceName.c_str());
	auto l_tempNameL = std::wstring(l_tempName.begin(), l_tempName.end());

	for (size_t i = 0; i < DX12RenderPassComp->m_CommandLists.size(); i++)
	{
		auto l_CommandList = reinterpret_cast<DX12CommandList *>(DX12RenderPassComp->m_CommandLists[i]);
		DX12RenderingServerNS::CreateCommandList(l_CommandList, i, l_tempNameL);
	}
}

bool TryToTransitState(DX12TextureComponent *rhs, DX12CommandList *commandList, const D3D12_RESOURCE_STATES& newState)
{
	if (rhs->m_CurrentState != newState)
	{
		commandList->m_DirectCommandList->ResourceBarrier(1,
														  &CD3DX12_RESOURCE_BARRIER::Transition(
															  rhs->m_DefaultHeapBuffer.Get(), rhs->m_CurrentState, newState));
		rhs->m_CurrentState = newState;
		return true;
	}

	return false;
}

bool DX12RenderingServerNS::CreateDebugCallback()
{
	ID3D12Debug *l_debugInterface;

	auto l_HResult = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't get DirectX 12 debug interface!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	l_HResult = l_debugInterface->QueryInterface(IID_PPV_ARGS(&m_debugInterface));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't query DirectX 12 debug interface!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_debugInterface->EnableDebugLayer();
	// m_debugInterface->SetEnableGPUBasedValidation(true);

	Log(Success, "Debug layer and GPU based validation has been enabled.");

	l_HResult = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_graphicsAnalysis));
	if (SUCCEEDED(l_HResult))
	{
		Log(Success, "PIX attached.");
	}

	return true;
}

bool DX12RenderingServerNS::CreatePhysicalDevices()
{
	// Create a DirectX graphics interface factory.
	UINT l_DXGIFlag = 0;
#ifdef INNO_DEBUG
	l_DXGIFlag |= DXGI_CREATE_FACTORY_DEBUG;
#endif // INNO_DEBUG

	auto l_HResult = CreateDXGIFactory2(l_DXGIFlag, IID_PPV_ARGS(&m_factory));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create DXGI factory!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	Log(Success, "DXGI factory has been created.");

	// Choose a dedicated adapter
	IDXGIAdapter1 *l_adapter;
	UINT adapterIndex = 0;
	l_HResult = m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&l_adapter));
	if (FAILED(l_HResult))
	{
		Log(Warning, "Can't find a high-performance GPU.");
		l_HResult = m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&l_adapter));
		if (FAILED(l_HResult))
		{
			Log(Error, "Can't find any capable GPU!");
			m_ObjectStatus = ObjectStatus::Suspended;
			return false;
		}
	}

	// Check to see if the adapter supports Direct3D 12, but don't create the
	// actual device yet.
	if (FAILED(D3D12CreateDevice(l_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
	{
		Log(Error, "Adapter doesn't support DirectX 12!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	if (l_adapter == nullptr)
	{
		Log(Error, "Can't create a suitable video card adapter!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_adapter = reinterpret_cast<IDXGIAdapter4 *>(l_adapter);

	DXGI_ADAPTER_DESC3 l_adapter_desc;
	m_adapter->GetDesc3(&l_adapter_desc);
	std::wstring l_descL = std::wstring(l_adapter_desc.Description);

    int length = WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string l_desc(length, 0);
    WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, l_desc.data(), length, nullptr, nullptr);

	Log(Success, "Adapter for: ", l_desc.c_str(), " has been created.");

	// Enumerate the primary adapter output (monitor).
	IDXGIOutput *l_adapterOutput;
	l_HResult = m_adapter->EnumOutputs(0, &l_adapterOutput);
	if (FAILED(l_HResult))
	{
		Log(Warning, "the primary output of the adapter is not connected.");
		// @TODO: Find a way to enumerate until we get the actual monitor
	}
	else
	{
		l_HResult = l_adapterOutput->QueryInterface(IID_PPV_ARGS(&m_adapterOutput));
	}

	uint32_t l_numModes;
	uint64_t l_stringLength;

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	l_HResult = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, NULL);
	if (FAILED(l_HResult))
	{
		Log(Error, "can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	std::vector<DXGI_MODE_DESC1> l_displayModeList(l_numModes);

	// Now fill the display mode list structures.
	l_HResult = m_adapterOutput->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, &l_displayModeList[0]);
	if (FAILED(l_HResult))
	{
		Log(Error, "can't fill the display mode list structures!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	for (uint32_t i = 0; i < l_numModes; i++)
	{
		if (l_displayModeList[i].Width == l_screenResolution.x &&
			l_displayModeList[i].Height == l_screenResolution.y)
		{
			m_refreshRate.x = l_displayModeList[i].RefreshRate.Numerator;
			m_refreshRate.y = l_displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Get the adapter (video card) description.
	l_HResult = m_adapter->GetDesc(&m_adapterDesc);
	if (FAILED(l_HResult))
	{
		Log(Error, "can't get the video card adapter description!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int32_t)(m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	if (wcstombs_s(&l_stringLength, m_videoCardDescription, 128, m_adapterDesc.Description, 128) != 0)
	{
		Log(Error, "can't convert the name of the video card to a character array!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Release the display mode list.
	// displayModeList.clear();

	// Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
	// Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
	auto featureLevel = D3D_FEATURE_LEVEL_12_1;

	// Create the Direct3D 12 device.
	l_HResult = D3D12CreateDevice(m_adapter.Get(), featureLevel, IID_PPV_ARGS(&m_device));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create a DirectX 12.1 device. The default video card does not support DirectX 12.1!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        if (SUCCEEDED(m_device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS,
            &options,
            sizeof(options))))
        {
            if(!options.TypedUAVLoadAdditionalFormats)
				Log(Warning, "TypedUAVLoadAdditionalFormats is not supported, can't generate mipmap for sRGB textures.");
        }

	Log(Success, "D3D device has been created.");

	// Set debug report severity
	ComPtr<ID3D12InfoQueue> l_pInfoQueue;
	l_HResult = m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue));

	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	// l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

	return true;
}

bool DX12RenderingServerNS::CreateGlobalCommandQueues()
{
	// Set up the description of the command queues.
	D3D12_COMMAND_QUEUE_DESC l_graphicCommandQueueDesc = {};
	l_graphicCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	l_graphicCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	l_graphicCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	l_graphicCommandQueueDesc.NodeMask = 0;

	D3D12_COMMAND_QUEUE_DESC l_computeCommandQueueDesc = {};
	l_computeCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	l_computeCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	l_computeCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	l_computeCommandQueueDesc.NodeMask = 0;

	D3D12_COMMAND_QUEUE_DESC l_copyCommandQueueDesc = {};
	l_copyCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	l_copyCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	l_copyCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	l_copyCommandQueueDesc.NodeMask = 0;

	m_directCommandQueue = CreateCommandQueue(&l_graphicCommandQueueDesc, m_device, L"DirectCommandQueue");
	m_computeCommandQueue = CreateCommandQueue(&l_computeCommandQueueDesc, m_device, L"ComputeCommandQueue");
	m_copyCommandQueue = CreateCommandQueue(&l_copyCommandQueueDesc, m_device, L"CopyCommandQueue");

	Log(Success, "Global CommandQueues have been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalCommandAllocators()
{
	m_copyCommandAllocator = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, m_device, L"CopyCommandAllocator");

	for (size_t i = 0; i < m_swapChainImageCount; i++)
	{
		m_directCommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, m_device, (L"DirectCommandAllocator_" + std::to_wstring(i)).c_str());
		m_computeCommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_device, (L"ComputeCommandAllocator_" + std::to_wstring(i)).c_str());
	}

	Log(Success, "Global CommandAllocators have been created.");

	for (size_t i = 0; i < m_GlobalCommandLists.size(); i++)
	{
		m_GlobalCommandLists[i] = AddCommandList();
		DX12RenderingServerNS::CreateCommandList(m_GlobalCommandLists[i], i, L"GPUBufferCommandList");
	}

	Log(Success, "Global CommandLists have been created.");

	return true;
}

bool DX12RenderingServerNS::CreateSyncPrimitives()
{
	for (size_t i = 0; i < m_swapChainImageCount; i++)
	{
		if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_directCommandQueueFence[i]))))
		{
			Log(Error, "Can't create Fence for direct CommandQueue!");
			return false;
		}
		if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_computeCommandQueueFence[i]))))
		{
			Log(Error, "Can't create Fence for compute CommandQueue!");
			return false;
		}
		if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_copyCommandQueueFence[i]))))
		{
			Log(Error, "Can't create Fence for copy CommandQueue!");
			return false;
		}
	#ifdef INNO_DEBUG
		m_directCommandQueueFence[i]->SetName((L"DirectCommandQueueFence_" + std::to_wstring(i)).c_str());
		m_computeCommandQueueFence[i]->SetName((L"ComputeCommandQueueFence_" + std::to_wstring(i)).c_str());
		m_copyCommandQueueFence[i]->SetName((L"CopyCommandQueueFence_" + std::to_wstring(i)).c_str());
	#endif // INNO_DEBUG
	}
	Log(Verbose, "Fences for global CommandQueues have been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalCSUDescHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC l_CSUHeapDesc = {};

	l_CSUHeapDesc.NumDescriptors = 65536;
	l_CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	l_CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = m_device->CreateDescriptorHeap(&l_CSUHeapDesc, IID_PPV_ARGS(&m_CSUDescHeap));
	if (FAILED(l_result))
	{
		Log(Error, "Can't create shader-visible DescriptorHeap for CBV/SRV/UAV!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_CSUDescHeap->SetName(L"GlobalCSUDescHeap_ShaderVisible");

	m_CSUDescHeapCPUHandle = m_CSUDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_CSUDescHeapGPUHandle = m_CSUDescHeap->GetGPUDescriptorHandleForHeapStart();

	Log(Success, "Shader-visible DescriptorHeap for CBV/SRV/UAV has been created.");

	D3D12_DESCRIPTOR_HEAP_DESC l_ShaderNonVisibleCSUHeapDesc = {};

	l_ShaderNonVisibleCSUHeapDesc.NumDescriptors = 65536;
	l_ShaderNonVisibleCSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	l_ShaderNonVisibleCSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	l_result = m_device->CreateDescriptorHeap(&l_ShaderNonVisibleCSUHeapDesc, IID_PPV_ARGS(&m_ShaderNonVisibleCSUDescHeap));
	if (FAILED(l_result))
	{
		Log(Error, "Can't create shader-non-visible DescriptorHeap for CBV/SRV/UAV!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_ShaderNonVisibleCSUDescHeap->SetName(L"GlobalCSUDescHeap_ShaderNonVisible");

	m_ShaderNonVisibleCSUDescHeapCPUHandle = m_ShaderNonVisibleCSUDescHeap->GetCPUDescriptorHandleForHeapStart();

	Log(Success, "Shader-non-visible DescriptorHeap for CBV/SRV/UAV has been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalSamplerDescHeap()
{
	m_samplerDescHeapDesc.NumDescriptors = 128;
	m_samplerDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	m_samplerDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = m_device->CreateDescriptorHeap(&m_samplerDescHeapDesc, IID_PPV_ARGS(&m_samplerDescHeap));
	if (FAILED(l_result))
	{
		Log(Error, "Can't create DescriptorHeap for Sampler!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_samplerDescHeap->SetName(L"GlobalSamplerDescHeap");

	m_SamplerDescHeapCPUHandle = m_samplerDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_SamplerDescHeapGPUHandle = m_samplerDescHeap->GetGPUDescriptorHandleForHeapStart();

	Log(Success, "DescriptorHeap for Sampler has been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalRTVDescHeap()
{
	m_RTVDescHeapDesc.NumDescriptors = 256;
	m_RTVDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	m_RTVDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto l_HResult = m_device->CreateDescriptorHeap(&m_RTVDescHeapDesc, IID_PPV_ARGS(&m_RTVDescHeap));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create DescriptorHeap for RTV!");
		return false;
	}

	m_RTVDescHeap->SetName(L"GlobalRTVDescHeap");

	m_RTVDescHeapCPUHandle = m_RTVDescHeap->GetCPUDescriptorHandleForHeapStart();

	Log(Success, "DescriptorHeap for RTV has been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalDSVDescHeap()
{
	m_DSVDescHeapDesc.NumDescriptors = 256;
	m_DSVDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	m_DSVDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto l_HResult = m_device->CreateDescriptorHeap(&m_DSVDescHeapDesc, IID_PPV_ARGS(&m_DSVDescHeap));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create DescriptorHeap for DSV!");
		return false;
	}

	m_DSVDescHeap->SetName(L"GlobalDSVDescHeap");

	m_DSVDescHeapCPUHandle = m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart();

	Log(Success, "DescriptorHeap for DSV has been created.");

	return true;
}

bool DX12RenderingServerNS::CreateMipmapGenerator()
{
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	{
		CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		rootParameters[0].InitAsConstants(3, 0);
		rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

		ID3DBlob *signature;
		ID3DBlob *error;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_3DMipmapRootSignature));

		ShaderFilePath l_3DPath = "mipmapGenerator3D.comp/";

		D3D12_COMPUTE_PIPELINE_STATE_DESC l_3DPSODesc = {};
		l_3DPSODesc.pRootSignature = m_3DMipmapRootSignature;

#ifdef USE_DXIL
		std::vector<char> l_3DmipmapComputeShader;
		LoadShaderFile(l_3DmipmapComputeShader, l_3DPath);
		l_3DPSODesc.CS = {&l_3DmipmapComputeShader[0], l_3DmipmapComputeShader.size()};
#else
		ID3DBlob *l_3DmipmapComputeShader;
		LoadShaderFile(&l_3DmipmapComputeShader, ShaderStage::Compute, l_3DPath);
		l_3DPSODesc.CS = {reinterpret_cast<UINT8 *>(l_3DmipmapComputeShader->GetBufferPointer()), l_3DmipmapComputeShader->GetBufferSize()};
#endif
		m_device->CreateComputePipelineState(&l_3DPSODesc, IID_PPV_ARGS(&m_3DMipmapPSO));

		Log(Success, "Mipmap generator for 3D texture has been created.");
	}
	{
		CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		rootParameters[0].InitAsConstants(2, 0);
		rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

		ID3DBlob *signature;
		ID3DBlob *error;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_2DMipmapRootSignature));

		ShaderFilePath l_2DPath = "mipmapGenerator2D.comp/";
		D3D12_COMPUTE_PIPELINE_STATE_DESC l_2DPSODesc = {};
		l_2DPSODesc.pRootSignature = m_2DMipmapRootSignature;

#ifdef USE_DXIL
		std::vector<char> l_2DmipmapComputeShader;
		LoadShaderFile(l_2DmipmapComputeShader, l_2DPath);
		l_2DPSODesc.CS = {&l_2DmipmapComputeShader[0], l_2DmipmapComputeShader.size()};
#else
		ID3DBlob *l_2DmipmapComputeShader;
		LoadShaderFile(&l_2DmipmapComputeShader, ShaderStage::Compute, l_2DPath);
		l_2DPSODesc.CS = {reinterpret_cast<UINT8 *>(l_2DmipmapComputeShader->GetBufferPointer()), l_2DmipmapComputeShader->GetBufferSize()};
#endif
		m_device->CreateComputePipelineState(&l_2DPSODesc, IID_PPV_ARGS(&m_2DMipmapPSO));

		Log(Success, "Mipmap generator for 2D texture has been created.");
	}

	return true;
}

bool DX12RenderingServerNS::CreateSwapChain()
{
	// Set the swap chain to use double buffering.
	m_swapChainDesc.BufferCount = m_swapChainImageCount;

	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	// Set the width and height of the back buffer.
	m_swapChainDesc.Width = (UINT)l_screenResolution.x;
	m_swapChainDesc.Height = (UINT)l_screenResolution.y;

	// Set regular 32-bit surface for the back buffer.
	m_swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the usage of the back buffer.
	m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

	// Turn multisampling off.
	m_swapChainDesc.SampleDesc.Count = 1;
	m_swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	// @TODO: finish this feature

	// Discard the back buffer contents after presenting.
	m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Don't set the advanced flags.
	m_swapChainDesc.Flags = 0;

	m_swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	// Finally create the swap chain
	IDXGISwapChain1 *l_swapChain1;
	auto l_hResult = m_factory->CreateSwapChainForHwnd(
		m_directCommandQueue.Get(),
		reinterpret_cast<WinWindowSystem *>(g_Engine->getWindowSystem())->GetWindowHandle(),
		&m_swapChainDesc,
		nullptr,
		nullptr,
		&l_swapChain1);

	l_hResult = l_swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));

	if (FAILED(l_hResult))
	{
		Log(Error, "Can't create swap chain!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	Log(Success, "Swap chain has been created.");

	return true;
}

bool DX12RenderingServerNS::GetSwapChainImages()
{
	m_swapChainImages.resize(m_swapChainImageCount);
	
	for (size_t i = 0; i < m_swapChainImageCount; i++)
	{
		auto l_HResult = m_swapChain->GetBuffer((uint32_t)i, IID_PPV_ARGS(&m_swapChainImages[i]));
		if (FAILED(l_HResult))
		{
			Log(Error, "Can't get pointer of swap chain image ", i, "!");
			return false;
		}
		m_swapChainImages[i]->SetName((L"SwapChainBackBuffer_" + std::to_wstring(i)).c_str());
	}

    return true;
}

bool DX12RenderingServerNS::AssignSwapChainImages()
{
	m_SwapChainRenderPassComp->m_RenderTargets.resize(m_swapChainImageCount);

	for (size_t i = 0; i < m_swapChainImageCount; i++)
	{
		auto l_DX12TextureComp = reinterpret_cast<DX12TextureComponent*>(m_SwapChainRenderPassComp->m_RenderTargets[i].m_Texture);

		l_DX12TextureComp->m_DefaultHeapBuffer = m_swapChainImages[i];
		l_DX12TextureComp->m_DX12TextureDesc = l_DX12TextureComp->m_DefaultHeapBuffer->GetDesc();
		l_DX12TextureComp->m_WriteState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		l_DX12TextureComp->m_ReadState = D3D12_RESOURCE_STATE_PRESENT;
		l_DX12TextureComp->m_CurrentState = l_DX12TextureComp->m_ReadState;

		l_DX12TextureComp->m_ObjectStatus = ObjectStatus::Activated;
	}

    return true;
}

bool DX12RenderingServerNS::GenerateMipmapImpl(DX12TextureComponent *DX12TextureComp)
{
	struct DWParam
	{
		DWParam(FLOAT f) : Float(f) {}
		DWParam(UINT u) : Uint(u) {}

		void operator=(FLOAT f) { Float = f; }
		void operator=(UINT u) { Uint = u; }

		union
		{
			FLOAT Float;
			UINT Uint;
		};
	};

	if (!DX12TextureComp->m_TextureDesc.UseMipMap)
	{
		Log(Warning, "Attempt to generate mipmaps for texture without mipmaps requirement.");

		return false;
	}

	if (!(DX12TextureComp->m_CurrentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
	{
		auto directCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
		directCommandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);
		directCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DX12TextureComp->m_DefaultHeapBuffer.Get(), DX12TextureComp->m_CurrentState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		DX12Helper::ExecuteCommandList(directCommandList, m_device, m_directCommandQueue);
	}

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ID3D12DescriptorHeap *l_heaps[] = {m_CSUDescHeap.Get()};

	auto computeCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_ComputeCommandList;
	computeCommandList->Reset(m_computeCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	if (DX12TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
	{
		computeCommandList->SetComputeRootSignature(m_3DMipmapRootSignature);
		computeCommandList->SetPipelineState(m_3DMipmapPSO);
	}
	else
	{
		computeCommandList->SetComputeRootSignature(m_2DMipmapRootSignature);
		computeCommandList->SetPipelineState(m_2DMipmapPSO);
	}
	computeCommandList->SetDescriptorHeaps(1, l_heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE l_SRV = DX12TextureComp->m_SRV.GPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE l_UAV;
	l_UAV.ptr = DX12TextureComp->m_UAV.ShaderVisibleGPUHandle.ptr + l_CSUDescSize;

	for (uint32_t TopMip = 0; TopMip < 4; TopMip++)
	{
		uint32_t dstWidth = std::max(DX12TextureComp->m_TextureDesc.Width >> (TopMip + 1), (uint32_t)1);
		uint32_t dstHeight = std::max(DX12TextureComp->m_TextureDesc.Height >> (TopMip + 1), (uint32_t)1);
		uint32_t dstDepth = 1;

		computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
		computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

		if (DX12TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
		{
			dstDepth = std::max(DX12TextureComp->m_TextureDesc.DepthOrArraySize >> (TopMip + 1), (uint32_t)1);
			computeCommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstDepth).Uint, 2);
		}

		computeCommandList->SetComputeRootDescriptorTable(1, l_SRV);
		computeCommandList->SetComputeRootDescriptorTable(2, l_UAV);

		computeCommandList->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), std::max(dstDepth / 8, 1u));

		l_SRV.ptr += l_CSUDescSize;
		l_UAV.ptr += l_CSUDescSize;
	}

	ExecuteCommandList(computeCommandList, m_device, m_computeCommandQueue);

	if (!(DX12TextureComp->m_CurrentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
	{
		auto directCommandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
		directCommandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);		
		directCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DX12TextureComp->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DX12TextureComp->m_CurrentState));
		DX12Helper::ExecuteCommandList(directCommandList, m_device, m_directCommandQueue);
	}

	return true;
}

using namespace DX12RenderingServerNS;

bool DX12RenderingServerNS::DeleteResources(DX12RenderPassComponent* rhs, DX12RenderingServer* renderingServer)
{
	if (rhs->m_RenderPassDesc.m_Resizable)
	{
		for (auto i : rhs->m_RenderTargets)
		{
			if(i.m_IsOwned)
				renderingServer->DeleteTextureComponent(i.m_Texture);
		}

		rhs->m_RenderTargets.clear();

		if (rhs->m_DepthStencilRenderTarget.m_IsOwned && rhs->m_DepthStencilRenderTarget.m_Texture)
		{
			renderingServer->DeleteTextureComponent(rhs->m_DepthStencilRenderTarget.m_Texture);
		}

		m_PSOPool->Destroy(reinterpret_cast<DX12PipelineStateObject*>(rhs->m_PipelineStateObject));
	}

	return true;
}

bool DX12RenderingServerNS::Resize(const TVec2<uint32_t>& screenResolution, DX12RenderPassComponent* rhs, DX12RenderingServer* renderingServer)
{
	if (rhs->m_RenderPassDesc.m_Resizable)
	{
		rhs->m_RenderPassDesc.m_RenderTargetDesc.Width = screenResolution.x;
		rhs->m_RenderPassDesc.m_RenderTargetDesc.Height = screenResolution.y;

		rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)screenResolution.x;
		rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)screenResolution.y;

		ReserveRenderTargets(rhs, renderingServer);

		CreateRenderTargets(rhs, renderingServer);

		if (rhs->m_RenderPassDesc.m_UseOutputMerger)
		{
			for (size_t i = 0; i < rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_RenderTargets[i].m_Texture)->m_DefaultHeapBuffer;
				m_device->CreateRenderTargetView(l_ResourceHandle.Get(), &rhs->m_RTVDesc, rhs->m_RTVDescCPUHandles[i]);
			}
		}

		if (rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			if (rhs->m_DepthStencilRenderTarget.m_Texture != nullptr)
			{
				auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_DepthStencilRenderTarget.m_Texture)->m_DefaultHeapBuffer;
				m_device->CreateDepthStencilView(l_ResourceHandle.Get(), &rhs->m_DSVDesc, rhs->m_DSVDescCPUHandle);
			}
		}

		rhs->m_PipelineStateObject = AddPSO();

		CreatePSO(rhs, m_device);
	}

	return true;
}

bool DX12RenderingServerNS::ResizeImpl()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	
	m_swapChainDesc.Width = (UINT)l_screenResolution.x;
	m_swapChainDesc.Height = (UINT)l_screenResolution.y;
	
	for (auto i : m_initializedRenderPasses)
	{
		auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(i);
		if (!DeleteResources(l_rhs, l_renderingServer))
			return false;
	}

	m_swapChainImages.clear();

	for (int i = 0; i < m_swapChainImageCount; i++)
    {
		UINT64 l_directCommandFinishedSemaphore = ++m_directCommandQueueSemaphore[i];
        m_directCommandQueue->Signal(m_directCommandQueueFence[i].Get(), l_directCommandFinishedSemaphore);
        if (m_directCommandQueueFence[i]->GetCompletedValue() < l_directCommandFinishedSemaphore)
        {
           	m_directCommandQueueFence[i]->SetEventOnCompletion(l_directCommandFinishedSemaphore, m_directCommandQueueFenceEvent[i]);
            WaitForSingleObject(m_directCommandQueueFenceEvent[i], INFINITE);
        }
    }

	m_swapChain->ResizeBuffers(m_swapChainImageCount, m_swapChainDesc.Width, m_swapChainDesc.Height, m_swapChainDesc.Format, 0);

	if (!GetSwapChainImages())
		return false;

	for (auto i : m_initializedRenderPasses)
	{
		auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(i);
		if (!Resize(l_screenResolution, l_rhs, l_renderingServer))
			return false;
	}
	
	return true;
}

bool DX12RenderingServer::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	m_MeshComponentPool = TObjectPool<DX12MeshComponent>::Create(l_renderingCapability.maxMeshes);
	m_TextureComponentPool = TObjectPool<DX12TextureComponent>::Create(l_renderingCapability.maxTextures);
	m_MaterialComponentPool = TObjectPool<DX12MaterialComponent>::Create(l_renderingCapability.maxMaterials);
	m_RenderPassComponentPool = TObjectPool<DX12RenderPassComponent>::Create(128);
	m_PSOPool = TObjectPool<DX12PipelineStateObject>::Create(128);
	m_CommandListPool = TObjectPool<DX12CommandList>::Create(256);
	m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(256);
	m_ShaderProgramComponentPool = TObjectPool<DX12ShaderProgramComponent>::Create(256);
	m_SamplerComponentPool = TObjectPool<DX12SamplerComponent>::Create(256);
	m_GPUBufferComponentPool = TObjectPool<DX12GPUBufferComponent>::Create(256);

	bool l_result = true;

#ifdef INNO_DEBUG
	l_result &= CreateDebugCallback();
#endif
	l_result &= CreatePhysicalDevices();
	l_result &= CreateGlobalCommandQueues();
	l_result &= CreateGlobalCommandAllocators();
	l_result &= CreateSyncPrimitives();
	l_result &= CreateGlobalCSUDescHeap();
	l_result &= CreateGlobalRTVDescHeap();
	l_result &= CreateGlobalDSVDescHeap();
	l_result &= CreateGlobalSamplerDescHeap();
	l_result &= CreateMipmapGenerator();
	l_result &= CreateSwapChain();

	m_SwapChainRenderPassComp = reinterpret_cast<DX12RenderPassComponent *>(AddRenderPassComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<DX12ShaderProgramComponent *>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSamplerComp = reinterpret_cast<DX12SamplerComponent *>(AddSamplerComponent("SwapChain/"));

	m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "DX12RenderingServer Setup finished.");

	return l_result;
}

bool DX12RenderingServer::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
		m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

		InitializeShaderProgramComponent(m_SwapChainSPC);

		InitializeSamplerComponent(m_SwapChainSamplerComp);

		if(!GetSwapChainImages())
			return false;

		auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

		l_RenderPassDesc.m_RenderTargetCount = m_swapChainImageCount;
		l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&AssignSwapChainImages);

		m_SwapChainRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
		m_SwapChainRenderPassComp->m_RenderPassDesc.m_UseMultiFrames = true;
		m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
		m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs.resize(2);
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_SubresourceCount = 1;
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Sampler;
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
		m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

		m_SwapChainRenderPassComp->m_ShaderProgram = m_SwapChainSPC;

		InitializeRenderPassComponent(m_SwapChainRenderPassComp);
		
		return true;
	}

	return false;
}

bool DX12RenderingServer::Terminate()
{
	DeleteSamplerComponent(m_SwapChainSamplerComp);
	DeleteShaderProgramComponent(m_SwapChainSPC);
	// DeleteRenderPassComponent(m_SwapChainRenderPassComp);

#ifdef INNO_DEBUG
	IDXGIDebug1 *pDebug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		pDebug->Release();
	}
#endif

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "DX12RenderingServer has been terminated.");

	return true;
}

ObjectStatus DX12RenderingServer::GetStatus()
{
	return m_ObjectStatus;
}

AddComponent(DX12, Mesh);
AddComponent(DX12, Texture);
AddComponent(DX12, Material);
AddComponent(DX12, RenderPass);
AddComponent(DX12, ShaderProgram);
AddComponent(DX12, Sampler);
AddComponent(DX12, GPUBuffer);

bool DX12RenderingServer::InitializeMeshComponent(MeshComponent *rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<DX12MeshComponent *>(rhs);

	// vertices
	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * l_rhs->m_Vertices.size());
	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);
	l_rhs->m_DefaultHeapBuffer_VB = CreateDefaultHeapBuffer(&l_verticesResourceDesc, m_device);
	if (l_rhs->m_DefaultHeapBuffer_VB == nullptr)
	{
		Log(Error, "can't create vertex buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_VB, "DefaultHeap_VB");
#endif //  INNO_DEBUG

	l_rhs->m_UploadHeapBuffer_VB = CreateUploadHeapBuffer(&l_verticesResourceDesc, m_device);
	if (l_rhs->m_UploadHeapBuffer_VB == nullptr)
	{
		Log(Error, "can't create vertex buffer on Upload Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_UploadHeapBuffer_VB, "UploadHeap_VB");
#endif //  INNO_DEBUG
	
	// Initialize the vertex buffer view.
	l_rhs->m_VBV.BufferLocation = l_rhs->m_DefaultHeapBuffer_VB->GetGPUVirtualAddress();
	l_rhs->m_VBV.StrideInBytes = sizeof(Vertex);
	l_rhs->m_VBV.SizeInBytes = l_verticesDataSize;

	Log(Verbose, "Vertex Buffer ", l_rhs->m_DefaultHeapBuffer_VB, " is initialized.");

	// indices
	auto l_indicesDataSize = uint32_t(sizeof(Index) * l_rhs->m_Indices.size());
	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);
	l_rhs->m_DefaultHeapBuffer_IB = CreateDefaultHeapBuffer(&l_indicesResourceDesc, m_device);
	if (l_rhs->m_DefaultHeapBuffer_IB == nullptr)
	{
		Log(Error, "can't create index buffer on Default Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_IB, "DefaultHeap_IB");
#endif //  INNO_DEBUG

	l_rhs->m_UploadHeapBuffer_IB = CreateUploadHeapBuffer(&l_indicesResourceDesc, m_device);
	if (l_rhs->m_UploadHeapBuffer_IB == nullptr)
	{
		Log(Error, "can't create index buffer on Upload Heap!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer_IB, "UploadHeap_IB");
#endif //  INNO_DEBUG

	// Initialize the index buffer view.
	l_rhs->m_IBV.Format = DXGI_FORMAT_R32_UINT;
	l_rhs->m_IBV.BufferLocation = l_rhs->m_DefaultHeapBuffer_IB->GetGPUVirtualAddress();
	l_rhs->m_IBV.SizeInBytes = l_indicesDataSize;

	Log(Verbose, "Index Buffer ", l_rhs->m_DefaultHeapBuffer_IB, " is initialized.");

	UpdateMeshComponent(l_rhs);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(rhs);

	return true;
}

bool DX12RenderingServer::InitializeTextureComponent(TextureComponent *rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		Log(Warning, "Texture ", rhs->m_InstanceName.c_str(), " has already been initialized!");
		return true;
	}

	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	l_rhs->m_DX12TextureDesc = GetDX12TextureDesc(l_rhs->m_TextureDesc);
	l_rhs->m_PixelDataSize = GetTexturePixelDataSize(l_rhs->m_TextureDesc);
	l_rhs->m_WriteState = GetTextureWriteState(l_rhs->m_TextureDesc);
	l_rhs->m_ReadState = GetTextureReadState(l_rhs->m_TextureDesc);

	if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_rhs->m_CurrentState = l_rhs->m_WriteState;
	}
	else
	{
		l_rhs->m_CurrentState = l_rhs->m_ReadState;
	}

	// Create the empty texture.
	if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		D3D12_CLEAR_VALUE l_clearValue;

		if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
		{
			l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
			l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		}
		else if (l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
		{
			l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		}
		else
		{
			l_clearValue.Format = l_rhs->m_DX12TextureDesc.Format;
			l_clearValue.Color[0] = l_rhs->m_TextureDesc.ClearColor[0];
			l_clearValue.Color[1] = l_rhs->m_TextureDesc.ClearColor[1];
			l_clearValue.Color[2] = l_rhs->m_TextureDesc.ClearColor[2];
			l_clearValue.Color[3] = l_rhs->m_TextureDesc.ClearColor[3];
		}
		l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, m_device, &l_clearValue);
	}
	else
	{
		l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDesc, m_device);
	}

	if (l_rhs->m_DefaultHeapBuffer == nullptr)
	{
		Log(Error, "can't create texture!");
		return false;
	}
#ifdef INNO_DEBUG
	SetObjectName(l_rhs, l_rhs->m_DefaultHeapBuffer, "DefaultHeap_Texture");
#endif // INNO_DEBUG

	auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	if (l_rhs->m_TextureData)
	{
		uint32_t l_subresourcesCount = l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : 1;
		UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(l_rhs->m_DefaultHeapBuffer.Get(), 0, l_subresourcesCount);

		l_rhs->m_UploadHeapBuffers.resize(l_subresourcesCount);

		for (uint32_t i = 0; i < l_subresourcesCount; i++)
		{
			D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
			l_textureSubResourceData.RowPitch = l_rhs->m_TextureDesc.Width * l_rhs->m_PixelDataSize;
			l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * l_rhs->m_TextureDesc.Height;
			l_textureSubResourceData.pData = (unsigned char*)l_rhs->m_TextureData + l_textureSubResourceData.RowPitch * l_rhs->m_TextureDesc.Height * i;

			auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
			l_rhs->m_UploadHeapBuffers[i] = CreateUploadHeapBuffer(&l_resourceDesc, m_device);
			UpdateSubresources(l_commandList.Get(), l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_UploadHeapBuffers[i].Get(), 0, i, 1, &l_textureSubResourceData);
		}
	}

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_rhs->m_CurrentState));
	DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);

	// Create SRV and UAV
	l_rhs->m_SRV = CreateSRV(l_rhs, 0);

	if (l_rhs->m_TextureDesc.UseMipMap)
	{
		for (uint32_t TopMip = 1; TopMip < 4; TopMip++)
		{
			auto l_SRV = CreateSRV(l_rhs, TopMip);
		}
	}

	if (l_rhs->m_TextureDesc.Usage != TextureUsage::DepthAttachment && l_rhs->m_TextureDesc.Usage != TextureUsage::DepthStencilAttachment)
	{
		if (!l_rhs->m_TextureDesc.IsSRGB)
		{
			l_rhs->m_UAV = CreateUAV(l_rhs, 0);

			if (l_rhs->m_TextureDesc.UseMipMap)
			{
				for (uint32_t TopMip = 0; TopMip < 4; TopMip++)
				{
					auto l_UAV = CreateUAV(l_rhs, TopMip + 1);
				}
			}
		}
	}

	if (l_rhs->m_TextureDesc.UseMipMap)
	{
		GenerateMipmap(l_rhs);
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Image;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(rhs);

	Log(Verbose, "texture ", l_rhs, " is initialized.");

	return true;
}

bool DX12RenderingServer::InitializeMaterialComponent(MaterialComponent *rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<DX12MaterialComponent *>(rhs);
	auto l_defaultMaterial = g_Engine->Get<TemplateAssetService>()->GetDefaultMaterialComponent();

	for (size_t i = 0; i < 8; i++)
	{
		auto l_texture = reinterpret_cast<DX12TextureComponent *>(l_rhs->m_TextureSlots[i].m_Texture);

		if (l_texture)
		{
			InitializeTextureComponent(l_texture);
			l_rhs->m_TextureSlots[i].m_Texture = l_texture;
			l_rhs->m_TextureSlots[i].m_Activate = true;
		}
		else
		{
			l_rhs->m_TextureSlots[i].m_Texture = l_defaultMaterial->m_TextureSlots[i].m_Texture;
		}
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(rhs);

	return true;
}

bool DX12RenderingServer::InitializeRenderPassComponent(RenderPassComponent *rhs)
{
	// @TODO: Move the tracker to the frontend
	auto it = std::find(m_initializedRenderPasses.begin(), m_initializedRenderPasses.end(), rhs);
	if (it != m_initializedRenderPasses.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);

	bool l_result = true;

	l_result &= ReserveRenderTargets(l_rhs, this);

	l_result &= CreateRenderTargets(l_rhs, this);

	if(l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_result &= CreateRTV(l_rhs);

		l_result &= CreateDSV(l_rhs);
	}

	l_result &= CreateRootSignature(l_rhs, m_device);

	l_rhs->m_PipelineStateObject = AddPSO();

	l_result &= CreatePSO(l_rhs, m_device);

	if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		l_rhs->m_CommandLists.resize(l_rhs->m_RenderPassDesc.m_RenderTargetCount);
	}
	else
	{
		l_rhs->m_CommandLists.resize(1);
	}

	for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
	{
		l_rhs->m_CommandLists[i] = AddCommandList();
	}

	CreateCommandLists(l_rhs);
	Log(Verbose, "", l_rhs->m_InstanceName.c_str(), " CommandList has been created.");

	l_rhs->m_Semaphores.resize(l_rhs->m_CommandLists.size());
	for (size_t i = 0; i < l_rhs->m_Semaphores.size(); i++)
	{
		l_rhs->m_Semaphores[i] = AddSemaphore();
	}
	
	Log(Verbose, "", l_rhs->m_InstanceName.c_str(), " Semaphore has been created.");

	CreateFenceEvents(l_rhs);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedRenderPasses.emplace_back(rhs);

	return l_result;
}

bool DX12RenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent *>(rhs);
#ifdef USE_DXIL
	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		LoadShaderFile(l_rhs->m_VSBuffer, l_rhs->m_ShaderFilePaths.m_VSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		LoadShaderFile(l_rhs->m_HSBuffer, l_rhs->m_ShaderFilePaths.m_HSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		LoadShaderFile(l_rhs->m_DSBuffer, l_rhs->m_ShaderFilePaths.m_DSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(l_rhs->m_GSBuffer, l_rhs->m_ShaderFilePaths.m_GSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		LoadShaderFile(l_rhs->m_PSBuffer, l_rhs->m_ShaderFilePaths.m_PSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(l_rhs->m_CSBuffer, l_rhs->m_ShaderFilePaths.m_CSPath);
	}
#else
	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		LoadShaderFile(&l_rhs->m_VSBuffer, ShaderStage::Vertex, l_rhs->m_ShaderFilePaths.m_VSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		LoadShaderFile(&l_rhs->m_HSBuffer, ShaderStage::Hull, l_rhs->m_ShaderFilePaths.m_HSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		LoadShaderFile(&l_rhs->m_DSBuffer, ShaderStage::Domain, l_rhs->m_ShaderFilePaths.m_DSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(&l_rhs->m_GSBuffer, ShaderStage::Geometry, l_rhs->m_ShaderFilePaths.m_GSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		LoadShaderFile(&l_rhs->m_PSBuffer, ShaderStage::Pixel, l_rhs->m_ShaderFilePaths.m_PSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(&l_rhs->m_CSBuffer, ShaderStage::Compute, l_rhs->m_ShaderFilePaths.m_CSPath);
	}
#endif
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeSamplerComponent(SamplerComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerComponent *>(rhs);

	l_rhs->m_Sampler.SamplerDesc.Filter = GetFilterMode(l_rhs->m_SamplerDesc.m_MinFilterMethod, l_rhs->m_SamplerDesc.m_MagFilterMethod);
	l_rhs->m_Sampler.SamplerDesc.AddressU = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodU);
	l_rhs->m_Sampler.SamplerDesc.AddressV = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodV);
	l_rhs->m_Sampler.SamplerDesc.AddressW = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodW);
	l_rhs->m_Sampler.SamplerDesc.MipLODBias = 0.0f;
	l_rhs->m_Sampler.SamplerDesc.MaxAnisotropy = l_rhs->m_SamplerDesc.m_MaxAnisotropy;
	l_rhs->m_Sampler.SamplerDesc.BorderColor[0] = l_rhs->m_SamplerDesc.m_BorderColor[0];
	l_rhs->m_Sampler.SamplerDesc.BorderColor[1] = l_rhs->m_SamplerDesc.m_BorderColor[1];
	l_rhs->m_Sampler.SamplerDesc.BorderColor[2] = l_rhs->m_SamplerDesc.m_BorderColor[2];
	l_rhs->m_Sampler.SamplerDesc.BorderColor[3] = l_rhs->m_SamplerDesc.m_BorderColor[3];
	l_rhs->m_Sampler.SamplerDesc.MinLOD = l_rhs->m_SamplerDesc.m_MinLOD;
	l_rhs->m_Sampler.SamplerDesc.MaxLOD = l_rhs->m_SamplerDesc.m_MaxLOD;

	l_rhs->m_Sampler.CPUHandle = m_SamplerDescHeapCPUHandle;
	l_rhs->m_Sampler.GPUHandle = m_SamplerDescHeapGPUHandle;

	m_device->CreateSampler(&l_rhs->m_Sampler.SamplerDesc, l_rhs->m_Sampler.CPUHandle);

	auto l_samplerDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	m_SamplerDescHeapCPUHandle.ptr += l_samplerDescSize;
	m_SamplerDescHeapGPUHandle.ptr += l_samplerDescSize;

	l_rhs->m_GPUResourceType = GPUResourceType::Sampler;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeGPUBufferComponent(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;
	l_rhs->m_GPUAccessibility = l_rhs->m_GPUAccessibility;
	l_rhs->m_ElementCount = l_rhs->m_ElementCount;
	l_rhs->m_ElementSize = l_rhs->m_ElementSize;
	l_rhs->m_TotalSize = l_rhs->m_TotalSize;

	auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize);
	l_rhs->m_UploadHeapBuffer = CreateUploadHeapBuffer(&l_resourceDesc, m_device);

#ifdef INNO_DEBUG
	SetObjectName(rhs, l_rhs->m_UploadHeapBuffer, "UploadHeap_General");
#endif // INNO_DEBUG

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		if (l_rhs->m_CPUAccessibility == Accessibility::Immutable || l_rhs->m_CPUAccessibility == Accessibility::WriteOnly)
		{
			auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			l_rhs->m_DefaultHeapBuffer = CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc, m_device);
#ifdef INNO_DEBUG
			SetObjectName(rhs, l_rhs->m_DefaultHeapBuffer, "DefaultHeap_General");
#endif // INNO_DEBUG

			auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
			l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);
			l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);

			l_rhs->m_UAV = CreateUAV(l_rhs);
		}
		else
		{
			Log(Warning, "Not support CPU-readable default heap GPU buffer currently.");
		}
	}

	l_rhs->m_SRV = CreateSRV(l_rhs);

	CD3DX12_RANGE m_readRange(0, 0);
	l_rhs->m_UploadHeapBuffer->Map(0, &m_readRange, &l_rhs->m_MappedMemory);

	if (l_rhs->m_InitialData)
	{
		UploadGPUBufferComponent(l_rhs, l_rhs->m_InitialData);
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Buffer;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::DeleteMeshComponent(MeshComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent *>(rhs);
	if(l_rhs->m_DefaultHeapBuffer_VB)
		l_rhs->m_DefaultHeapBuffer_VB.Reset();

	if(l_rhs->m_DefaultHeapBuffer_IB)
		l_rhs->m_DefaultHeapBuffer_IB.Reset();

	if(l_rhs->m_UploadHeapBuffer_VB)
		l_rhs->m_UploadHeapBuffer_VB.Reset();

	if(l_rhs->m_UploadHeapBuffer_IB)
		l_rhs->m_UploadHeapBuffer_IB.Reset();

	m_MeshComponentPool->Destroy(l_rhs);

	m_initializedMeshes.erase(rhs);

	return true;
}

bool DX12RenderingServer::DeleteTextureComponent(TextureComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	if(l_rhs->m_DefaultHeapBuffer)
		l_rhs->m_DefaultHeapBuffer.Reset();

	m_TextureComponentPool->Destroy(l_rhs);

	m_initializedTextures.erase(rhs);

	return true;
}

bool DX12RenderingServer::DeleteMaterialComponent(MaterialComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12MaterialComponent *>(rhs);

	m_MaterialComponentPool->Destroy(l_rhs);

	m_initializedMaterials.erase(rhs);

	return true;
}

bool DX12RenderingServer::DeleteRenderPassComponent(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_rhs->m_PipelineStateObject);

	m_PSOPool->Destroy(l_PSO);

	if (l_rhs->m_DepthStencilRenderTarget.m_Texture)
	{
		DeleteTextureComponent(l_rhs->m_DepthStencilRenderTarget.m_Texture);
	}

	for (size_t i = 0; i < l_rhs->m_RenderTargets.size(); i++)
	{
		DeleteTextureComponent(l_rhs->m_RenderTargets[i].m_Texture);
	}

	m_RenderPassComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent *>(rhs);

	m_ShaderProgramComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteSamplerComponent(SamplerComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerComponent *>(rhs);

	m_SamplerComponentPool->Destroy(l_rhs);

	return false;
}

bool DX12RenderingServer::DeleteGPUBufferComponent(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	if(l_rhs->m_DefaultHeapBuffer)
		l_rhs->m_DefaultHeapBuffer.Reset();

	if(l_rhs->m_UploadHeapBuffer)
		l_rhs->m_UploadHeapBuffer.Reset();
		
	m_GPUBufferComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::UpdateMeshComponent(MeshComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshComponent *>(rhs);
	
	// Flip y texture coordinate
	for (auto &i : rhs->m_Vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}

	CD3DX12_RANGE m_readRange(0, 0);
	l_rhs->m_UploadHeapBuffer_VB->Map(0, &m_readRange, &l_rhs->m_MappedUploadHeapBuffer_VB);
	std::memcpy((char *)l_rhs->m_MappedUploadHeapBuffer_VB, &l_rhs->m_Vertices[0], l_rhs->m_Vertices.size() * sizeof(Vertex));

	l_rhs->m_UploadHeapBuffer_IB->Map(0, &m_readRange, &l_rhs->m_MappedUploadHeapBuffer_IB);
	std::memcpy((char *)l_rhs->m_MappedUploadHeapBuffer_IB, &l_rhs->m_Indices[0], l_rhs->m_Indices.size() * sizeof(Index));

	auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	if(l_rhs->m_ObjectStatus == ObjectStatus::Activated)
	{
		l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
		l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	}
	l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer_VB.Get(), l_rhs->m_UploadHeapBuffer_VB.Get());
	l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer_IB.Get(), l_rhs->m_UploadHeapBuffer_IB.Get());
	l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_VB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	l_commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer_IB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);

	return true;
}

bool DX12RenderingServer::ClearTextureComponent(TextureComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);
	
	ID3D12DescriptorHeap *l_heaps[] = {m_CSUDescHeap.Get()};
	l_commandList->SetDescriptorHeaps(1, l_heaps);
	
	if(l_rhs->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_CurrentState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	if (l_rhs->m_TextureDesc.PixelDataType < TexturePixelDataType::Float16)
	{
		l_commandList->ClearUnorderedAccessViewUint(
			l_rhs->m_UAV.ShaderVisibleGPUHandle,
			l_rhs->m_UAV.ShaderNonVisibleCPUHandle,
			l_rhs->m_DefaultHeapBuffer.Get(),
			(UINT *)&l_rhs->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}
	else
	{
		l_commandList->ClearUnorderedAccessViewFloat(
			l_rhs->m_UAV.ShaderVisibleGPUHandle,
			l_rhs->m_UAV.ShaderNonVisibleCPUHandle,
			l_rhs->m_DefaultHeapBuffer.Get(),
			&l_rhs->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}

	if(l_rhs->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, l_rhs->m_CurrentState));

	DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);

	return true;
}

bool DX12RenderingServer::CopyTextureComponent(TextureComponent *lhs, TextureComponent *rhs)
{
	auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	auto l_src = reinterpret_cast<DX12TextureComponent *>(lhs);
	auto l_dest = reinterpret_cast<DX12TextureComponent *>(rhs);

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_DefaultHeapBuffer.Get(), l_src->m_CurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE));

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_DefaultHeapBuffer.Get(), l_dest->m_CurrentState, D3D12_RESOURCE_STATE_COPY_DEST));

	l_commandList->CopyResource(l_dest->m_DefaultHeapBuffer.Get(), l_src->m_DefaultHeapBuffer.Get());

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, l_src->m_CurrentState));

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_dest->m_CurrentState));

	DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);

	return true;
}

bool DX12RenderingServer::UploadGPUBufferComponentImpl(GPUBufferComponent *rhs, const void *GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	auto l_size = l_rhs->m_TotalSize;
	if (range != SIZE_MAX)
	{
		l_size = range * l_rhs->m_ElementSize;
	}

	std::memcpy((char *)l_rhs->m_MappedMemory + startOffset * l_rhs->m_ElementSize, GPUBufferValue, l_size);
	DX12RenderingServerNS::m_dirtyBuffers.emplace_back(l_rhs);

	return true;
}

bool DX12RenderingServer::ClearGPUBufferComponent(GPUBufferComponent *rhs)
{
	const uint32_t zero = 0;
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	ID3D12DescriptorHeap *l_heaps[] = {m_CSUDescHeap.Get()};
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	l_commandList->ClearUnorderedAccessViewUint(
		l_rhs->m_UAV.ShaderVisibleGPUHandle,
		l_rhs->m_UAV.ShaderNonVisibleCPUHandle,
		l_rhs->m_DefaultHeapBuffer.Get(),
		&zero,
		0,
		NULL);

	DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);

	return true;
}

void DX12RenderingServer::TransferDataToGPU()
{
	if (m_dirtyBuffers.empty())
		return;

	auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
	l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

	for (auto rhs : m_dirtyBuffers)
	{
		auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

		if (!l_rhs->m_DefaultHeapBuffer)
			continue;
		
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
		l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_UploadHeapBuffer.Get());
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);
	
	m_dirtyBuffers.clear();
}

bool DX12RenderingServer::CommandListBegin(RenderPassComponent *rhs, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[frameIndex]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_rhs->m_PipelineStateObject);

	l_rhs->m_CurrentFrame = frameIndex;

	l_commandList->m_DirectCommandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), l_PSO->m_PSO.Get());
	l_commandList->m_ComputeCommandList->Reset(m_computeCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), l_PSO->m_PSO.Get());

	return true;
}

bool PrepareRenderTargets(DX12RenderPassComponent *renderPass, DX12CommandList *commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
		{
			auto l_rhs = reinterpret_cast<DX12TextureComponent *>(renderPass->m_RenderTargets[renderPass->m_CurrentFrame].m_Texture);

			TryToTransitState(l_rhs, commandList, l_rhs->m_WriteState);
		}
		else
		{
			for (size_t i = 0; i < renderPass->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				auto l_rhs = reinterpret_cast<DX12TextureComponent *>(renderPass->m_RenderTargets[i].m_Texture);

				TryToTransitState(l_rhs, commandList, l_rhs->m_WriteState);
			}
		}

		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
		{
			auto l_rhs = reinterpret_cast<DX12TextureComponent *>(renderPass->m_DepthStencilRenderTarget.m_Texture);

			TryToTransitState(l_rhs, commandList, l_rhs->m_WriteState);
		}
	}

	return true;
}

bool PreparePipeline(DX12RenderPassComponent *renderPass, DX12CommandList *commandList, DX12PipelineStateObject *PSO)
{
	ID3D12DescriptorHeap *l_heaps[] = {m_CSUDescHeap.Get(), m_samplerDescHeap.Get()};

	ComPtr<ID3D12GraphicsCommandList> l_commandList;
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_commandList = commandList->m_DirectCommandList;
	}
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandList = commandList->m_ComputeCommandList;
	}

	l_commandList->SetDescriptorHeaps(2, l_heaps);
	l_commandList->SetPipelineState(PSO->m_PSO.Get());

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_commandList->SetGraphicsRootSignature(renderPass->m_RootSignature.Get());
		l_commandList->RSSetViewports(1, &PSO->m_Viewport);
		l_commandList->RSSetScissorRects(1, &PSO->m_Scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE *l_DSV = NULL;

		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			l_DSV = &renderPass->m_DSVDescCPUHandle;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE *l_RTVs = NULL;
		uint32_t l_RTCount = 0;

		if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
		{
			if (renderPass->m_RenderPassDesc.m_RenderTargetCount)
			{
				if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
				{
					l_RTVs = &renderPass->m_RTVDescCPUHandles[renderPass->m_CurrentFrame];
					l_RTCount = 1;
				}
				else
				{
					l_RTVs = &renderPass->m_RTVDescCPUHandles[0];
					l_RTCount = (uint32_t)renderPass->m_RenderPassDesc.m_RenderTargetCount;
				}
			}
		}

		l_commandList->OMSetRenderTargets(l_RTCount, l_RTVs, FALSE, l_DSV);

		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable)
		{
			l_commandList->OMSetStencilRef(renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference);
		}
	}
	else
	{
		l_commandList->SetComputeRootSignature(renderPass->m_RootSignature.Get());
	}

	return true;
}

bool DX12RenderingServer::BindRenderPassComponent(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_rhs->m_PipelineStateObject);

	PrepareRenderTargets(l_rhs, l_commandList);
	PreparePipeline(l_rhs, l_commandList, l_PSO);

	for	(size_t i = 0; i < l_rhs->m_ResourceBindingLayoutDescs.size(); i++)
	{
		auto& l_desc = l_rhs->m_ResourceBindingLayoutDescs[i];
		if(l_desc.m_GPUResource)
		{
			BindGPUResource(rhs, l_desc.m_ShaderStage, l_desc.m_GPUResource, i);
		}
	}
	
	return true;
}

bool DX12RenderingServer::ClearRenderTargets(RenderPassComponent *rhs, size_t index)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics && rhs->m_RenderPassDesc.m_RenderTargetCount)
	{
		auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
		auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
		auto f_clearRTAsUAV = [&](DX12TextureComponent* l_RT)
		{
			if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
			{
				l_commandList->m_DirectCommandList->ClearUnorderedAccessViewUint(
					l_RT->m_UAV.ShaderVisibleGPUHandle,
					l_RT->m_UAV.ShaderNonVisibleCPUHandle,
					l_RT->m_DefaultHeapBuffer.Get(),
					(UINT*)l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0,
					NULL);
			}
			else
			{
				l_commandList->m_DirectCommandList->ClearUnorderedAccessViewFloat(
					l_RT->m_UAV.ShaderVisibleGPUHandle,
					l_RT->m_UAV.ShaderNonVisibleCPUHandle,
					l_RT->m_DefaultHeapBuffer.Get(),
					l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0,
					NULL);
			}
		};

		if (l_rhs->m_RenderPassDesc.m_UseOutputMerger)
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[l_rhs->m_CurrentFrame], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
			}
			else
			{
				if(index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
				{
					l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[index], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
				}
				else
				{
					for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
					{
						l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[i], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
					}
				}
			}
		}
		else
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame].m_Texture));
			}
			else
			{
				if (index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
				{
					f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(index));
				}
				else
				{

					for (auto i : l_rhs->m_RenderTargets)
					{
						f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(i.m_Texture));
					}
				}
			}
		}

		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
		{
			auto l_flag = D3D12_CLEAR_FLAG_DEPTH;
			if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite)
			{
				l_flag |= D3D12_CLEAR_FLAG_STENCIL;
			}
			l_commandList->m_DirectCommandList->ClearDepthStencilView(l_rhs->m_DSVDescCPUHandle, l_flag, 1.0f, 0x00, 0, nullptr);
		}
	}

	return true;
}

bool DX12RenderingServer::BindGPUResource(RenderPassComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if ((l_renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute && shaderStage != ShaderStage::Compute) || (l_renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Compute && shaderStage == ShaderStage::Compute))
	{
		Log(Warning, "Trying to activate resource at : ", resourceBindingLayoutDescIndex, " with incompatible render pass: ", renderPass->m_InstanceName.c_str());
		return false;
	}

	if (!resource)
	{
		Log(Warning, "Empty resource resource in render pass: ", renderPass->m_InstanceName.c_str(), ", at: ", resourceBindingLayoutDescIndex);
		return false;
	}

	auto l_accessibility = l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex].m_BindingAccessibility;
	auto l_bindingType = l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex].m_GPUResourceType;
	auto l_resourceType = resource->m_GPUResourceType;

	DX12UAV l_UAV = {};
	DX12SRV l_SRV = {};

	if (l_resourceType == GPUResourceType::Image)
	{
		auto l_DX12TextureComp = reinterpret_cast<DX12TextureComponent*>(resource);

		l_UAV = l_DX12TextureComp->m_UAV;
		l_SRV = l_DX12TextureComp->m_SRV;

		if (l_accessibility != Accessibility::ReadOnly)
		{
			// @TODO: Let the client do these
			TryToTransitState(l_DX12TextureComp, l_commandList, l_DX12TextureComp->m_WriteState);
		}
		else
		{
			TryToTransitState(l_DX12TextureComp, l_commandList, l_DX12TextureComp->m_ReadState);
		}
	}
	else if (l_resourceType == GPUResourceType::Buffer)
	{
		auto l_DX12GPUBufferComp = reinterpret_cast<DX12GPUBufferComponent*>(resource);

		l_UAV = l_DX12GPUBufferComp->m_UAV;
		l_SRV = l_DX12GPUBufferComp->m_SRV;
	}

	if (shaderStage == ShaderStage::Compute)
	{
		switch (l_bindingType)
		{
		case GPUResourceType::Sampler:
			l_commandList->m_ComputeCommandList->SetComputeRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12SamplerComponent*>(resource)->m_Sampler.GPUHandle);
			break;
		case GPUResourceType::Image:
		{
			if (l_accessibility != Accessibility::ReadOnly)
			{
				l_commandList->m_ComputeCommandList->SetComputeRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, l_UAV.ShaderVisibleGPUHandle);
			}
			else
			{
				l_commandList->m_ComputeCommandList->SetComputeRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, l_SRV.GPUHandle);
			}
			break;
		}
		case GPUResourceType::Buffer:
			if (resource->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				if (l_accessibility != Accessibility::ReadOnly)
				{
					Log(Warning, "Not allow GPU write to Constant Buffer!");
				}
				else
				{
					l_commandList->m_ComputeCommandList->SetComputeRootConstantBufferView((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_UploadHeapBuffer->GetGPUVirtualAddress() + startOffset * reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_ElementSize);
				}
			}
			else
			{
				if (l_accessibility != Accessibility::ReadOnly)
				{
					if (reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_isAtomicCounter)
					{
						l_commandList->m_ComputeCommandList->SetComputeRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, l_UAV.ShaderVisibleGPUHandle);
					}
					else
					{
						l_commandList->m_ComputeCommandList->SetComputeRootUnorderedAccessView((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_ElementSize);
					}
				}
				else
				{
					l_commandList->m_ComputeCommandList->SetComputeRootShaderResourceView((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_ElementSize);
				}
			}

			break;
		default:
			break;
		}
	}
	else
	{
		switch (l_bindingType)
		{
		case GPUResourceType::Sampler:
			l_commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12SamplerComponent*>(resource)->m_Sampler.GPUHandle);
			break;
		case GPUResourceType::Image:
		{
			if (l_accessibility != Accessibility::ReadOnly)
			{
				l_commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, l_UAV.ShaderVisibleGPUHandle);
			}
			else
			{
				l_commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, l_SRV.GPUHandle);
			}
			break;
		}
		case GPUResourceType::Buffer:
			if (resource->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				if (l_accessibility != Accessibility::ReadOnly)
				{
					Log(Warning, "Not allow GPU write to Constant Buffer!");
				}
				else
				{
					l_commandList->m_DirectCommandList->SetGraphicsRootConstantBufferView((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_UploadHeapBuffer->GetGPUVirtualAddress() + startOffset * reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_ElementSize);
				}
			}
			else
			{
				if (l_accessibility != Accessibility::ReadOnly)
				{
					if (reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_isAtomicCounter)
					{
						l_commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable((uint32_t)resourceBindingLayoutDescIndex, l_UAV.ShaderVisibleGPUHandle);
					}
					else
					{
						l_commandList->m_DirectCommandList->SetGraphicsRootUnorderedAccessView((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_ElementSize);
					}
				}
				else
				{
					l_commandList->m_DirectCommandList->SetGraphicsRootShaderResourceView((uint32_t)resourceBindingLayoutDescIndex, reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * reinterpret_cast<DX12GPUBufferComponent*>(resource)->m_ElementSize);
				}
			}
			break;
		default:
			break;
		}
	}


	return true;
}

bool DX12RenderingServer::DrawIndexedInstanced(RenderPassComponent *renderPass, MeshComponent *mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<DX12MeshComponent *>(mesh);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, &l_mesh->m_VBV);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(&l_mesh->m_IBV);
	l_commandList->m_DirectCommandList->DrawIndexedInstanced((uint32_t)l_mesh->m_IndexCount, (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12RenderingServer::DrawInstanced(RenderPassComponent *renderPass, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(l_renderPass->m_PipelineStateObject);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, nullptr);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(nullptr);
	l_commandList->m_DirectCommandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12RenderingServer::UnbindGPUResource(RenderPassComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return true;
}

bool DX12RenderingServer::CommandListEnd(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	for	(size_t i = 0; i < l_rhs->m_ResourceBindingLayoutDescs.size(); i++)
	{
		auto& l_desc = l_rhs->m_ResourceBindingLayoutDescs[0];
		if(l_desc.m_GPUResource)
		{
			UnbindGPUResource(rhs, l_desc.m_ShaderStage, l_desc.m_GPUResource, i);
		}
	}

	l_commandList->m_DirectCommandList->Close();
	l_commandList->m_ComputeCommandList->Close();

	return true;
}

bool DX12RenderingServer::ExecuteCommandList(RenderPassComponent *rhs, GPUEngineType GPUEngineType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_semaphore = reinterpret_cast<DX12Semaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	if (GPUEngineType == GPUEngineType::Graphics)
	{
		UINT64 l_directCommandFinishedSemaphore = ++m_directCommandQueueSemaphore[l_rhs->m_CurrentFrame];
		m_directCommandQueueFence[l_rhs->m_CurrentFrame]->SetEventOnCompletion(l_directCommandFinishedSemaphore, l_semaphore->m_DirectCommandQueueFenceEvent);
		l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		ID3D12CommandList *l_directCommandLists[] = {l_commandList->m_DirectCommandList.Get()};
		m_directCommandQueue->ExecuteCommandLists(1, l_directCommandLists);

		m_directCommandQueueFenceEvent[l_rhs->m_CurrentFrame] = l_semaphore->m_DirectCommandQueueFenceEvent;
		m_directCommandQueue->Signal(m_directCommandQueueFence[l_rhs->m_CurrentFrame].Get(), l_directCommandFinishedSemaphore);
	}
	else if (GPUEngineType == GPUEngineType::Compute)
	{
		UINT64 l_computeCommandFinishedSemaphore = ++m_computeCommandQueueSemaphore[l_rhs->m_CurrentFrame];
		m_computeCommandQueueFence[l_rhs->m_CurrentFrame]->SetEventOnCompletion(l_computeCommandFinishedSemaphore, l_semaphore->m_ComputeCommandQueueFenceEvent);
		l_semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;

		ID3D12CommandList *l_computeCommandLists[] = {l_commandList->m_ComputeCommandList.Get()};
		m_computeCommandQueue->ExecuteCommandLists(1, l_computeCommandLists);

		m_computeCommandQueueFenceEvent[l_rhs->m_CurrentFrame] = l_semaphore->m_ComputeCommandQueueFenceEvent;
		m_computeCommandQueue->Signal(m_computeCommandQueueFence[l_rhs->m_CurrentFrame].Get(), l_computeCommandFinishedSemaphore);
	}

	return true;
}

bool DX12RenderingServer::WaitCommandQueue(RenderPassComponent *rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	auto l_semaphore = reinterpret_cast<DX12Semaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);
	ID3D12CommandQueue *commandQueue = nullptr;
	ID3D12Fence *fence = nullptr;
	uint64_t semaphore = 0;

	if (queueType == GPUEngineType::Graphics)
	{
		commandQueue = m_directCommandQueue.Get();
	}
	else if (queueType == GPUEngineType::Compute)
	{
		commandQueue = m_computeCommandQueue.Get();
	}

	if (semaphoreType == GPUEngineType::Graphics)
	{
		fence = m_directCommandQueueFence[l_rhs->m_CurrentFrame].Get();
		semaphore = l_semaphore->m_DirectCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		fence = m_computeCommandQueueFence[l_rhs->m_CurrentFrame].Get();
		semaphore = l_semaphore->m_ComputeCommandQueueSemaphore;
	}

	if (commandQueue && fence)
	{
		commandQueue->Wait(fence, semaphore);
	}

	return true;
}

bool DX12RenderingServer::WaitFence(RenderPassComponent *rhs, GPUEngineType GPUEngineType)
{
	UINT64 semaphore = 0;
	HANDLE fenceEvent = 0;
	auto l_currentFrame = m_SwapChainRenderPassComp->m_CurrentFrame;

	if (rhs == nullptr)
	{
		if (GPUEngineType == GPUEngineType::Graphics)
		{
			semaphore = m_directCommandQueueSemaphore[m_SwapChainRenderPassComp->m_CurrentFrame];
			fenceEvent = m_directCommandQueueFenceEvent[m_SwapChainRenderPassComp->m_CurrentFrame];
		}
		else if (GPUEngineType == GPUEngineType::Compute)
		{
			semaphore = m_computeCommandQueueSemaphore[m_SwapChainRenderPassComp->m_CurrentFrame];
			fenceEvent = m_computeCommandQueueFenceEvent[m_SwapChainRenderPassComp->m_CurrentFrame];
		}
	}
	else
	{
		auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
		auto l_semaphore = reinterpret_cast<DX12Semaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

		if (GPUEngineType == GPUEngineType::Graphics)
		{
			semaphore = l_semaphore->m_DirectCommandQueueSemaphore;
			fenceEvent = l_semaphore->m_DirectCommandQueueFenceEvent;
		}
		else if (GPUEngineType == GPUEngineType::Compute)
		{
			semaphore = l_semaphore->m_ComputeCommandQueueSemaphore;
			fenceEvent = l_semaphore->m_ComputeCommandQueueFenceEvent;
		}
	}

	if (GPUEngineType == GPUEngineType::Graphics)
	{
		if (m_directCommandQueueFence[l_currentFrame]->GetCompletedValue() < semaphore)
		{
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		ResetEvent(fenceEvent);
	}
	else if (GPUEngineType == GPUEngineType::Compute)
	{
		if (m_computeCommandQueueFence[l_currentFrame]->GetCompletedValue() < semaphore)
		{
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		ResetEvent(fenceEvent);
	}

	return true;
}

bool DX12RenderingServer::SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc)
{
	m_GetUserPipelineOutputFunc = getUserPipelineOutputFunc;

	return true;
}

GPUResourceComponent *DX12RenderingServer::GetUserPipelineOutput()
{
	return m_GetUserPipelineOutputFunc();
}

bool DX12RenderingServer::Present()
{
	auto l_commandList = reinterpret_cast<DX12CommandList *>(m_SwapChainRenderPassComp->m_CommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject *>(m_SwapChainRenderPassComp->m_PipelineStateObject);
	auto l_DX12TextureComp = reinterpret_cast<DX12TextureComponent *>(m_SwapChainRenderPassComp->m_RenderTargets[m_SwapChainRenderPassComp->m_CurrentFrame].m_Texture);
	auto l_semaphore = reinterpret_cast<DX12Semaphore *>(m_SwapChainRenderPassComp->m_Semaphores[m_SwapChainRenderPassComp->m_CurrentFrame]);

	CommandListBegin(m_SwapChainRenderPassComp, m_SwapChainRenderPassComp->m_CurrentFrame);

	BindRenderPassComponent(m_SwapChainRenderPassComp);

	ClearRenderTargets(m_SwapChainRenderPassComp);

	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_SwapChainSamplerComp, 1);

	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_GetUserPipelineOutputFunc(), 0);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(ProceduralMeshShape::Square);

	DrawIndexedInstanced(m_SwapChainRenderPassComp, l_mesh, 1);

	UnbindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_GetUserPipelineOutputFunc(), 0);

	TryToTransitState(l_DX12TextureComp, l_commandList,l_DX12TextureComp->m_ReadState);

	CommandListEnd(m_SwapChainRenderPassComp);

	UINT64 l_directCommandFinishedSemaphore = ++m_directCommandQueueSemaphore[m_SwapChainRenderPassComp->m_CurrentFrame];
	m_directCommandQueueFence[m_SwapChainRenderPassComp->m_CurrentFrame]->SetEventOnCompletion(l_directCommandFinishedSemaphore, l_semaphore->m_DirectCommandQueueFenceEvent);
	l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

	ID3D12CommandList *l_directCommandLists[] = {l_commandList->m_DirectCommandList.Get()};
	m_directCommandQueue->ExecuteCommandLists(1, l_directCommandLists);

	m_directCommandQueue->Signal(m_directCommandQueueFence[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), l_directCommandFinishedSemaphore);

	m_swapChain->Present(0, 0);

	WaitFence(m_SwapChainRenderPassComp, GPUEngineType::Graphics);
	WaitFence(m_SwapChainRenderPassComp, GPUEngineType::Compute);

	auto l_previousFrame = m_SwapChainRenderPassComp->m_CurrentFrame;
	m_SwapChainRenderPassComp->m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();

	m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame]->Reset();
	m_computeCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame]->Reset();
	m_copyCommandAllocator->Reset();

	if (m_needResize)
	{
		ResizeImpl();

		m_SwapChainRenderPassComp->m_CurrentFrame = 0;
		m_needResize = false;
	}

	return true;
}

bool DX12RenderingServer::Dispatch(RenderPassComponent *renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	l_commandList->m_ComputeCommandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

Vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassComponent *rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassComponent *canvas, TextureComponent *TextureComp)
{
	// @TODO: Support different pixel data type
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(TextureComp);
	auto l_srcDesc = l_rhs->m_DefaultHeapBuffer->GetDesc();

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints;
	l_footprints.resize(l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : l_rhs->m_TextureDesc.DepthOrArraySize);

	m_device->GetCopyableFootprints(&l_srcDesc, 0, (UINT)l_footprints.size(), 0, l_footprints.data(), NULL, NULL, NULL);

	if (!l_rhs->m_ReadBackHeapBuffer)
	{
		UINT64 bufferSize = 0;
		for (size_t i = 0; i < l_footprints.size(); ++i)
		{
			bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;
		}

		l_rhs->m_ReadBackHeapBuffer = CreateReadBackHeapBuffer(bufferSize, m_device);
#ifdef INNO_DEBUG
		SetObjectName(l_rhs, l_rhs->m_ReadBackHeapBuffer, "ReadBackHeap_Texture");
#endif // INNO_DEBUG
	}

	auto f_DefaultToReadbackHeap = [](ComPtr<ID3D12Resource> l_defaultHeapBuffer, ComPtr<ID3D12Resource> l_readbackHeapBuffer, const std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>& footprints, DXGI_FORMAT l_format, D3D12_RESOURCE_STATES currentState)
	{
		{
			auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_device, m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame]);

			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					currentState,
					D3D12_RESOURCE_STATE_COMMON));
			DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);
		}

		for (size_t i = 0; i < footprints.size(); i++)
		{
			D3D12_TEXTURE_COPY_LOCATION l_srcLocation = {};
			l_srcLocation.pResource = l_defaultHeapBuffer.Get();
			l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			l_srcLocation.SubresourceIndex = (UINT)i;

			D3D12_TEXTURE_COPY_LOCATION l_destLocation = {};
			l_destLocation.pResource = l_readbackHeapBuffer.Get();
			l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			l_destLocation.PlacedFootprint = footprints[i];
			l_destLocation.PlacedFootprint.Footprint.Format = l_format;

			auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_device, m_copyCommandAllocator);

			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					D3D12_RESOURCE_STATE_COMMON,
					D3D12_RESOURCE_STATE_COPY_SOURCE));

			l_commandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);

			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					D3D12_RESOURCE_STATE_COPY_SOURCE,
					D3D12_RESOURCE_STATE_COMMON));

			DX12Helper::ExecuteCommandList(l_commandList, m_device, m_copyCommandQueue);
		}

		{
			auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_device, m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame]);
			l_commandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
					D3D12_RESOURCE_STATE_COMMON,
					currentState));
			DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);
		}
	};

	size_t l_pixelCount = 0;
	
	auto textureDesc = l_rhs->m_TextureDesc;
	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_pixelCount = textureDesc.Width;
		break;
	case TextureSampler::Sampler2D:
		l_pixelCount = textureDesc.Width * textureDesc.Height;
		break;
	case TextureSampler::Sampler3D:
		l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler1DArray:
		l_pixelCount = textureDesc.Width * textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_pixelCount = textureDesc.Width * textureDesc.Height * 6;
		break;
	default:
		break;
	}

	auto f_ReadbackToHostHeap = [](ComPtr<ID3D12Resource> l_readbackHeapBuffer, uint32_t l_pixelDataSize, size_t l_pixelCount) -> std::vector<unsigned char>
	{
		std::vector<unsigned char> l_result;
		l_result.resize(l_pixelCount * l_pixelDataSize);

		CD3DX12_RANGE m_ReadRange(0, l_result.size());
		void *l_pData;
		auto l_HResult = l_readbackHeapBuffer->Map(0, &m_ReadRange, &l_pData);

		if (FAILED(l_HResult))
		{
			Log(Error, "Can't map texture for CPU to read!");
		}

		std::memcpy(l_result.data(), l_pData, l_result.size());
		l_readbackHeapBuffer->Unmap(0, 0);

		return l_result;
	};

	// Copy from default heap to readback heap, then copy from readback heap to application's heap region
	DXGI_FORMAT l_format;
	std::vector<Vec4> l_result;

	if (textureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
	{
		l_format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R8_TYPELESS for the stencil
		f_DefaultToReadbackHeap(l_rhs->m_DefaultHeapBuffer, l_rhs->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
		auto l_rawResult = f_ReadbackToHostHeap(l_rhs->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);
		auto l_pixelCount = l_rawResult.size() / 4;
		
		std::vector<uint32_t> l_resultUint32;
		l_resultUint32.resize(l_pixelCount);
		l_result.resize(l_pixelCount);

		std::memcpy(l_resultUint32.data(), l_rawResult.data(), l_rawResult.size());
		for (size_t i = 0; i < l_pixelCount; i++)
		{
			auto l_value = l_resultUint32[i];
			auto l_depth = l_value & 0x00FFFFFF;
			auto l_stencil = l_value & 0xFF000000;
			l_result[i].x = float(l_depth) / float(0x00FFFFFF);
			l_result[i].y = float(l_stencil);
		}
	}
	else
	{
		l_format = l_rhs->m_DX12TextureDesc.Format;
		f_DefaultToReadbackHeap(l_rhs->m_DefaultHeapBuffer, l_rhs->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
		auto l_rawResult = f_ReadbackToHostHeap(l_rhs->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);

		l_result.resize(l_pixelCount);
		if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
		{
			for (int i = 0; i < l_pixelCount; ++i)
			{
				const unsigned char *pixelData = &l_rawResult[i * 8];

				// Convert the RGBA FLOAT16 data to float values
				float floatData[4];
				for (int j = 0; j < 4; ++j)
				{
					const unsigned char *floatBytes = &pixelData[j * 2];
					unsigned short float16;
					memcpy(&float16, floatBytes, sizeof(unsigned short));
					floatData[j] = Math::float16ToFloat32(float16);
				}

				// Construct a Vec4 from the float values
				l_result[i] = Vec4(floatData[0], floatData[1], floatData[2], floatData[3]);
			}
		}
		else	
		{
			std::memcpy(l_result.data(), l_rawResult.data(), l_rawResult.size());
		}
	}

	return l_result;
}

bool DX12RenderingServer::GenerateMipmap(TextureComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	if(l_rhs->m_TextureDesc.IsSRGB)
	{
		auto l_copy = reinterpret_cast<DX12TextureComponent*>(AddTextureComponent((l_rhs->m_InstanceName.c_str() + std::string("_MipCopy/")).c_str()));
		l_copy->m_TextureDesc = l_rhs->m_TextureDesc;
		l_copy->m_TextureData = l_rhs->m_TextureData;
		l_copy->m_TextureDesc.IsSRGB = false;
		InitializeTextureComponent(l_copy);

		D3D12_RESOURCE_BARRIER barrier[2] = {};
		barrier[0].Type = barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier[0].Transition.Subresource = barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier[0].Transition.pResource = l_copy->m_DefaultHeapBuffer.Get();
		barrier[0].Transition.StateBefore = l_copy->m_CurrentState;
		barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		barrier[1].Transition.pResource = l_rhs->m_DefaultHeapBuffer.Get();
		barrier[1].Transition.StateBefore = l_rhs->m_CurrentState;
		barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		auto l_commandList = m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]->m_DirectCommandList;
		l_commandList->Reset(m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame].Get(), nullptr);

		l_commandList->ResourceBarrier(2, barrier);

		// Copy the entire resource back
		l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer.Get(), l_copy->m_DefaultHeapBuffer.Get());
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_rhs->m_CurrentState));

		DX12Helper::ExecuteCommandList(l_commandList, m_device, m_directCommandQueue);

		return true;
	}

	return GenerateMipmapImpl(l_rhs);
}

bool DX12RenderingServer::Resize()
{
	m_needResize = true;		
	return true;
}

bool DX12RenderingServer::CreateRTV(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	if (l_rhs->m_RenderPassDesc.m_UseOutputMerger)
	{
		auto l_RTVDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		l_rhs->m_RTVDesc = GetRTVDesc(l_rhs->m_RenderPassDesc.m_RenderTargetDesc);
		l_rhs->m_RTVDescCPUHandles.resize(l_rhs->m_RenderPassDesc.m_RenderTargetCount);

		for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			l_rhs->m_RTVDescCPUHandles[i] = m_RTVDescHeapCPUHandle;
			auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(l_rhs->m_RenderTargets[i].m_Texture)->m_DefaultHeapBuffer;
			m_device->CreateRenderTargetView(l_ResourceHandle.Get(), &l_rhs->m_RTVDesc, l_rhs->m_RTVDescCPUHandles[i]);

			m_RTVDescHeapCPUHandle.ptr += l_RTVDescSize;
		}

		Log(Verbose, "", l_rhs->m_InstanceName.c_str(), " RTV has been created.");
		return true;
	}

	return false;
}

bool DX12RenderingServer::CreateDSV(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent *>(rhs);
	if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		if (l_rhs->m_DepthStencilRenderTarget.m_Texture != nullptr)
		{
			auto l_DSVDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			l_rhs->m_DSVDesc = GetDSVDesc(l_rhs->m_RenderPassDesc.m_RenderTargetDesc, l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable);
			l_rhs->m_DSVDescCPUHandle = m_DSVDescHeapCPUHandle;

			auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(l_rhs->m_DepthStencilRenderTarget.m_Texture)->m_DefaultHeapBuffer;
			m_device->CreateDepthStencilView(l_ResourceHandle.Get(), &l_rhs->m_DSVDesc, l_rhs->m_DSVDescCPUHandle);
			
			m_DSVDescHeapCPUHandle.ptr += l_DSVDescSize;

			Log(Verbose, "", l_rhs->m_InstanceName.c_str(), " DSV has been created.");
			return false;
		}
		else
		{
			Log(Error, "", l_rhs->m_InstanceName.c_str(), " depth (and stencil) test is enable, but no depth-stencil render target is bound!");
			return false;
		}
	}

	return false;
}

DX12SRV CreateSRVImpl(D3D12_SHADER_RESOURCE_VIEW_DESC desc, ComPtr<ID3D12Resource> resourceHandle)
{
	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	DX12SRV l_result = {};
	l_result.SRVDesc = desc;
	l_result.CPUHandle = m_CSUDescHeapCPUHandle;
	l_result.GPUHandle = m_CSUDescHeapGPUHandle;

	m_CSUDescHeapCPUHandle.ptr += l_CSUDescSize;
	m_CSUDescHeapGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateShaderResourceView(resourceHandle.Get(), &l_result.SRVDesc, l_result.CPUHandle);

	return l_result;
}

DX12SRV DX12RenderingServer::CreateSRV(TextureComponent *rhs, uint32_t mostDetailedMip)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);
	return CreateSRVImpl(GetSRVDesc(l_rhs->m_TextureDesc, l_rhs->m_DX12TextureDesc, mostDetailedMip), l_rhs->m_DefaultHeapBuffer);
}

DX12SRV DX12RenderingServer::CreateSRV(GPUBufferComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = l_rhs->m_isAtomicCounter ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_UINT;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)l_rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = l_rhs->m_isAtomicCounter ? (uint32_t)l_rhs->m_ElementSize : 0;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    return CreateSRVImpl(l_desc, l_rhs->m_DefaultHeapBuffer);
}

DX12UAV CreateUAVImpl(D3D12_UNORDERED_ACCESS_VIEW_DESC desc, ComPtr<ID3D12Resource> resourceHandle, bool isAtomicCounter)
{
	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	DX12UAV l_result = {};
	l_result.UAVDesc = desc;
	l_result.ShaderNonVisibleCPUHandle = m_ShaderNonVisibleCSUDescHeapCPUHandle;
	l_result.ShaderVisibleGPUHandle = m_CSUDescHeapGPUHandle;

	auto l_CSUDescHeapCPUHandle = m_CSUDescHeapCPUHandle;

	m_ShaderNonVisibleCSUDescHeapCPUHandle.ptr += l_CSUDescSize;
	m_CSUDescHeapCPUHandle.ptr += l_CSUDescSize;
	m_CSUDescHeapGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateUnorderedAccessView(resourceHandle.Get(), isAtomicCounter ? resourceHandle.Get() : 0, &l_result.UAVDesc, l_result.ShaderNonVisibleCPUHandle);
	m_device->CreateUnorderedAccessView(resourceHandle.Get(), isAtomicCounter ? resourceHandle.Get() : 0, &l_result.UAVDesc, l_CSUDescHeapCPUHandle);
	
	return l_result;
}

DX12UAV DX12RenderingServer::CreateUAV(TextureComponent *rhs, uint32_t mipSlice)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	auto l_desc = GetUAVDesc(l_rhs->m_TextureDesc, l_rhs->m_DX12TextureDesc, mipSlice);

	return CreateUAVImpl(l_desc, l_rhs->m_DefaultHeapBuffer, false);
}

DX12UAV DX12RenderingServer::CreateUAV(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = l_rhs->m_isAtomicCounter ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_UINT;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = (uint32_t)l_rhs->m_ElementCount;
	l_desc.Buffer.StructureByteStride = l_rhs->m_isAtomicCounter ? (uint32_t)l_rhs->m_ElementSize : 0;

	return CreateUAVImpl(l_desc, l_rhs->m_DefaultHeapBuffer, l_rhs->m_isAtomicCounter);
}

DX12CBV DX12RenderingServer::CreateCBV(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent *>(rhs);

	DX12CBV l_result;

	l_result.CBVDesc.BufferLocation = l_rhs->m_UploadHeapBuffer->GetGPUVirtualAddress();
	l_result.CBVDesc.SizeInBytes = (uint32_t)l_rhs->m_ElementSize;

	l_result.CPUHandle = m_CSUDescHeapCPUHandle;
	l_result.GPUHandle = m_CSUDescHeapGPUHandle;

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_CSUDescHeapCPUHandle.ptr += l_CSUDescSize;
	m_CSUDescHeapGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateConstantBufferView(&l_result.CBVDesc, l_result.CPUHandle);

	return l_result;
}

ID3D12Device* DX12RenderingServer::GetDevice()
{
    return m_device.Get();
}

ID3D12DescriptorHeap* DX12RenderingServer::GetCSUDescHeap()
{
    return m_CSUDescHeap.Get();
}

ID3D12CommandAllocator* DX12RenderingServer::GetCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, uint32_t swapChainImageIndex)
{
    switch (commandListType)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return m_directCommandAllocators[swapChainImageIndex].Get();
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return m_computeCommandAllocators[swapChainImageIndex].Get();
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return m_copyCommandAllocator.Get();
        case D3D12_COMMAND_LIST_TYPE_BUNDLE:
        default:
            throw std::runtime_error("Invalid command list type");
    }
}

uint32_t DX12RenderingServer::GetSwapChainImageCount()
{
    return m_swapChainImageCount;
}

bool DX12RenderingServer::BeginCapture()
{
	if (m_graphicsAnalysis != nullptr)
	{
		m_graphicsAnalysis->BeginCapture();
		return true;
	}

	return false;
}

bool DX12RenderingServer::EndCapture()
{
	if (m_graphicsAnalysis != nullptr)
	{
		m_graphicsAnalysis->EndCapture();
		return true;
	}

	return false;
}
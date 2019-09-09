#include "DX12RenderingServer.h"
#include "../../Component/DX12MeshDataComponent.h"
#include "../../Component/DX12TextureDataComponent.h"
#include "../../Component/DX12MaterialDataComponent.h"
#include "../../Component/DX12RenderPassDataComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../../Component/DX12SamplerDataComponent.h"
#include "../../Component/DX12GPUBufferDataComponent.h"

#include "../../Component/WinWindowSystemComponent.h"

#include "DX12Helper.h"

using namespace DX12Helper;

#include "../CommonFunctionDefinationMacro.inl"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

#include "../../Core/InnoLogger.h"
#include "../../Core/InnoMemory.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace DX12RenderingServerNS
{
	bool CreateDebugCallback();
	bool CreatePhysicalDevices();
	bool CreateGlobalCommandQueue();
	bool CreateGlobalCommandAllocator();
	bool CreateGlobalCSUHeap();
	bool CreateGlobalSamplerHeap();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	IObjectPool* m_MeshDataComponentPool = 0;
	IObjectPool* m_MaterialDataComponentPool = 0;
	IObjectPool* m_TextureDataComponentPool = 0;
	IObjectPool* m_RenderPassDataComponentPool = 0;
	IObjectPool* m_ResourcesBinderPool = 0;
	IObjectPool* m_PSOPool = 0;
	IObjectPool* m_CommandQueuePool = 0;
	IObjectPool* m_CommandListPool = 0;
	IObjectPool* m_FencePool = 0;
	IObjectPool* m_ShaderProgramComponentPool = 0;
	IObjectPool* m_SamplerDataComponentPool = 0;
	IObjectPool* m_GPUBufferDataComponentPool = 0;

	std::unordered_set<MeshDataComponent*> m_initializedMeshes;
	std::unordered_set<TextureDataComponent*> m_initializedTextures;
	std::unordered_set<MaterialDataComponent*> m_initializedMaterials;

	TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);

	int32_t m_videoCardMemory = 0;
	char m_videoCardDescription[128];

	ID3D12Debug1* m_debugInterface = 0;

	IDXGIFactory4* m_factory = 0;

	DXGI_ADAPTER_DESC m_adapterDesc = {};
	IDXGIAdapter4* m_adapter = 0;
	IDXGIOutput1* m_adapterOutput = 0;

	ID3D12Device2* m_device = 0;

	ID3D12CommandAllocator* m_globalCommandAllocator = 0;
	D3D12_COMMAND_QUEUE_DESC m_globalCommandQueueDesc = {};
	ID3D12CommandQueue* m_globalCommandQueue = 0;

	DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc = {};
	IDXGISwapChain4* m_swapChain = 0;
	const uint32_t m_swapChainImageCount = 2;
	std::vector<ID3D12Resource*> m_swapChainImages(m_swapChainImageCount);

	ID3D12DescriptorHeap* m_CSUHeap = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE m_initialCSUCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_initialCSUGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentCSUCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_currentCSUGPUHandle;

	ID3D12DescriptorHeap* m_ShaderNonVisibleCSUHeap = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE m_initialShaderNonVisibleCSUCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_initialShaderNonVisibleCSUGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentShaderNonVisibleCSUCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_currentShaderNonVisibleCSUGPUHandle;

	ID3D12DescriptorHeap* m_samplerHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC m_samplerHeapDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_initialSamplerCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_initialSamplerGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentSamplerCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_currentSamplerGPUHandle;

	DX12RenderPassDataComponent* m_userPipelineOutput = 0;
	DX12RenderPassDataComponent* m_SwapChainRPDC = 0;
	DX12ShaderProgramComponent* m_SwapChainSPC = 0;
	DX12SamplerDataComponent* m_SwapChainSDC = 0;
}

bool DX12RenderingServerNS::CreateDebugCallback()
{
	ID3D12Debug* l_debugInterface;

	auto l_HResult = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't get DirectX 12 debug interface!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	l_HResult = l_debugInterface->QueryInterface(IID_PPV_ARGS(&m_debugInterface));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't query DirectX 12 debug interface!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_debugInterface->EnableDebugLayer();
	//m_debugInterface->SetEnableGPUBasedValidation(true);

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: Debug layer and GPU based validation has been enabled.");

	return true;
}

bool DX12RenderingServerNS::CreatePhysicalDevices()
{
	// Create a DirectX graphics interface factory.
	auto l_HResult = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_factory));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create DXGI factory!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: DXGI factory has been created.");

	// Use the factory to create an adapter for the primary graphics interface (video card).
	IDXGIAdapter1* l_adapter1;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &l_adapter1); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 l_adapterDesc;
		l_adapter1->GetDesc1(&l_adapterDesc);

		if (l_adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(l_adapter1, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	if (l_adapter1 == nullptr)
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create a suitable video card adapter!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_adapter = reinterpret_cast<IDXGIAdapter4*>(l_adapter1);

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: Video card adapter has been created.");

	// Enumerate the primary adapter output (monitor).
	IDXGIOutput* l_adapterOutput;

	l_HResult = m_adapter->EnumOutputs(0, &l_adapterOutput);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't create monitor adapter!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	l_HResult = l_adapterOutput->QueryInterface(IID_PPV_ARGS(&m_adapterOutput));

	uint32_t l_numModes;
	uint64_t l_stringLength;

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	l_HResult = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, NULL);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	std::vector<DXGI_MODE_DESC1> l_displayModeList(l_numModes);

	// Now fill the display mode list structures.
	l_HResult = m_adapterOutput->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, &l_displayModeList[0]);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't fill the display mode list structures!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	for (uint32_t i = 0; i < l_numModes; i++)
	{
		if (l_displayModeList[i].Width == l_screenResolution.x
			&&
			l_displayModeList[i].Height == l_screenResolution.y
			)
		{
			m_refreshRate.x = l_displayModeList[i].RefreshRate.Numerator;
			m_refreshRate.y = l_displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Get the adapter (video card) description.
	l_HResult = m_adapter->GetDesc(&m_adapterDesc);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't get the video card adapter description!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int32_t)(m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	if (wcstombs_s(&l_stringLength, m_videoCardDescription, 128, m_adapterDesc.Description, 128) != 0)
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't convert the name of the video card to a character array!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Release the display mode list.
	// displayModeList.clear();

	// Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
	// Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
	auto featureLevel = D3D_FEATURE_LEVEL_12_1;

	// Create the Direct3D 12 device.
	l_HResult = D3D12CreateDevice(m_adapter, featureLevel, IID_PPV_ARGS(&m_device));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create a DirectX 12.1 device. The default video card does not support DirectX 12.1!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: D3D device has been created.");

	// Set debug report severity
	ID3D12InfoQueue* l_pInfoQueue;
	l_HResult = m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue));

	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	//l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

	return true;
}

bool DX12RenderingServerNS::CreateGlobalCommandQueue()
{
	// Set up the description of the command queue.
	m_globalCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	m_globalCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	m_globalCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	m_globalCommandQueueDesc.NodeMask = 0;

	// Create the command queue.
	auto l_HResult = m_device->CreateCommandQueue(&m_globalCommandQueueDesc, IID_PPV_ARGS(&m_globalCommandQueue));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create global CommandQueue!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_globalCommandQueue->SetName(L"GlobalCommandQueue");

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: Global CommandQueue has been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalCommandAllocator()
{
	auto l_HResult = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_globalCommandAllocator));
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create global CommandAllocator!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_globalCommandAllocator->SetName(L"GlobalCommandAllocator");

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: Global CommandAllocator has been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalCSUHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC l_CSUHeapDesc = {};

	l_CSUHeapDesc.NumDescriptors = 65536;
	l_CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	l_CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = m_device->CreateDescriptorHeap(&l_CSUHeapDesc, IID_PPV_ARGS(&m_CSUHeap));
	if (FAILED(l_result))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create shader-visible DescriptorHeap for CBV/SRV/UAV!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_CSUHeap->SetName(L"ShaderVisibleGlobalCSUHeap");

	m_initialCSUCPUHandle = m_CSUHeap->GetCPUDescriptorHandleForHeapStart();
	m_initialCSUGPUHandle = m_CSUHeap->GetGPUDescriptorHandleForHeapStart();

	m_currentCSUCPUHandle = m_initialCSUCPUHandle;
	m_currentCSUGPUHandle = m_initialCSUGPUHandle;

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: Shader-visible DescriptorHeap for CBV/SRV/UAV has been created.");

	D3D12_DESCRIPTOR_HEAP_DESC l_ShaderNonVisibleCSUHeapDesc = {};

	l_ShaderNonVisibleCSUHeapDesc.NumDescriptors = 65536;
	l_ShaderNonVisibleCSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	l_ShaderNonVisibleCSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	l_result = m_device->CreateDescriptorHeap(&l_ShaderNonVisibleCSUHeapDesc, IID_PPV_ARGS(&m_ShaderNonVisibleCSUHeap));
	if (FAILED(l_result))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create shader-non-visible DescriptorHeap for CBV/SRV/UAV!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_ShaderNonVisibleCSUHeap->SetName(L"ShaderNonVisibleGlobalCSUHeap");

	m_initialShaderNonVisibleCSUCPUHandle = m_ShaderNonVisibleCSUHeap->GetCPUDescriptorHandleForHeapStart();
	m_initialShaderNonVisibleCSUGPUHandle = m_ShaderNonVisibleCSUHeap->GetGPUDescriptorHandleForHeapStart();

	m_currentShaderNonVisibleCSUCPUHandle = m_initialShaderNonVisibleCSUCPUHandle;
	m_currentShaderNonVisibleCSUGPUHandle = m_initialShaderNonVisibleCSUGPUHandle;

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: Shader-non-visible DescriptorHeap for CBV/SRV/UAV has been created.");

	return true;
}

bool DX12RenderingServerNS::CreateGlobalSamplerHeap()
{
	m_samplerHeapDesc.NumDescriptors = 128;
	m_samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	m_samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = m_device->CreateDescriptorHeap(&m_samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap));
	if (FAILED(l_result))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create DescriptorHeap for Sampler!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_samplerHeap->SetName(L"GlobalSamplerHeap");

	m_initialSamplerCPUHandle = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
	m_initialSamplerGPUHandle = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();

	m_currentSamplerCPUHandle = m_initialSamplerCPUHandle;
	m_currentSamplerGPUHandle = m_initialSamplerGPUHandle;

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: DescriptorHeap for Sampler has been created.");

	return true;
}

using namespace DX12RenderingServerNS;

DX12ResourceBinder* addResourcesBinder()
{
	auto l_BinderRawPtr = m_ResourcesBinderPool->Spawn();
	auto l_Binder = new(l_BinderRawPtr)DX12ResourceBinder();
	return l_Binder;
}

DX12PipelineStateObject* addPSO()
{
	auto l_PSORawPtr = m_PSOPool->Spawn();
	auto l_PSO = new(l_PSORawPtr)DX12PipelineStateObject();
	return l_PSO;
}

DX12CommandQueue* addCommandQueue()
{
	auto l_commandQueueRawPtr = m_CommandQueuePool->Spawn();
	auto l_commandQueue = new(l_commandQueueRawPtr)DX12CommandQueue();
	return l_commandQueue;
}

DX12CommandList* addCommandList()
{
	auto l_commandListRawPtr = m_CommandListPool->Spawn();
	auto l_commandList = new(l_commandListRawPtr)DX12CommandList();
	return l_commandList;
}

DX12Fence* addFence()
{
	auto l_fenceRawPtr = m_FencePool->Spawn();
	auto l_fence = new(l_fenceRawPtr)DX12Fence();
	return l_fence;
}

bool DX12RenderingServer::Setup()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_MeshDataComponentPool = InnoMemory::CreateObjectPool(sizeof(DX12MeshDataComponent), l_renderingCapability.maxMeshes);
	m_TextureDataComponentPool = InnoMemory::CreateObjectPool(sizeof(DX12TextureDataComponent), l_renderingCapability.maxTextures);
	m_MaterialDataComponentPool = InnoMemory::CreateObjectPool(sizeof(DX12MaterialDataComponent), l_renderingCapability.maxMaterials);
	m_RenderPassDataComponentPool = InnoMemory::CreateObjectPool(sizeof(DX12RenderPassDataComponent), 128);
	m_ResourcesBinderPool = InnoMemory::CreateObjectPool(sizeof(DX12ResourceBinder), 16384);
	m_PSOPool = InnoMemory::CreateObjectPool(sizeof(DX12PipelineStateObject), 128);
	m_CommandQueuePool = InnoMemory::CreateObjectPool(sizeof(DX12CommandQueue), 128);
	m_CommandListPool = InnoMemory::CreateObjectPool(sizeof(DX12CommandList), 256);
	m_FencePool = InnoMemory::CreateObjectPool(sizeof(DX12Fence), 256);
	m_ShaderProgramComponentPool = InnoMemory::CreateObjectPool(sizeof(DX12ShaderProgramComponent), 256);
	m_SamplerDataComponentPool = InnoMemory::CreateObjectPool(sizeof(DX12SamplerDataComponent), 256);
	m_GPUBufferDataComponentPool = InnoMemory::CreateObjectPool(sizeof(DX12GPUBufferDataComponent), 256);

	bool l_result = true;

#ifdef _DEBUG
	l_result &= CreateDebugCallback();
#endif
	l_result &= CreatePhysicalDevices();
	l_result &= CreateGlobalCommandQueue();
	l_result &= CreateGlobalCommandAllocator();
	l_result &= CreateGlobalCSUHeap();
	l_result &= CreateGlobalSamplerHeap();

	m_SwapChainRPDC = reinterpret_cast<DX12RenderPassDataComponent*>(AddRenderPassDataComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<DX12ShaderProgramComponent*>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSDC = reinterpret_cast<DX12SamplerDataComponent*>(AddSamplerDataComponent("SwapChain/"));

	m_objectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer setup finished.");

	return l_result;
}

bool DX12RenderingServer::Initialize()
{
	if (m_objectStatus == ObjectStatus::Created)
	{
		m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
		m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

		InitializeShaderProgramComponent(m_SwapChainSPC);

		InitializeSamplerDataComponent(m_SwapChainSDC);

		// Create command queue first
		auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

		l_RenderPassDesc.m_RenderTargetCount = m_swapChainImageCount;

		m_SwapChainRPDC->m_RenderPassDesc = l_RenderPassDesc;
		m_SwapChainRPDC->m_RenderPassDesc.m_UseMultiFrames = true;
		m_SwapChainRPDC->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UBYTE;
		m_SwapChainRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

		m_SwapChainRPDC->m_ResourceBinderLayoutDescs.resize(2);
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceCount = 1;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_IsRanged = true;

		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Sampler;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 0;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_IsRanged = true;

		m_SwapChainRPDC->m_ShaderProgram = m_SwapChainSPC;

		ReserveRenderTargets(m_SwapChainRPDC, this);

		m_SwapChainRPDC->m_CommandQueue = addCommandQueue();

		CreateCommandQueue(m_SwapChainRPDC, m_device);
		CreateCommandAllocators(m_SwapChainRPDC, m_device);

		m_SwapChainRPDC->m_CommandLists.resize(m_SwapChainRPDC->m_CommandAllocators.size());
		for (size_t i = 0; i < m_SwapChainRPDC->m_CommandLists.size(); i++)
		{
			m_SwapChainRPDC->m_CommandLists[i] = addCommandList();
		}

		CreateCommandLists(m_SwapChainRPDC, m_device);

		// Create swap chain
		// Set the swap chain to use double buffering.
		m_swapChainDesc.BufferCount = m_swapChainImageCount;

		auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

		// Set the width and height of the back buffer.
		m_swapChainDesc.Width = (UINT)l_screenResolution.x;
		m_swapChainDesc.Height = (UINT)l_screenResolution.y;

		// Set regular 32-bit surface for the back buffer.
		m_swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// Set the usage of the back buffer.
		m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

		// Turn multisampling off.
		m_swapChainDesc.SampleDesc.Count = 1;
		m_swapChainDesc.SampleDesc.Quality = 0;

		// Set to full screen or windowed mode.
		// @TODO: finish this feature

		// Discard the back buffer contents after presenting.
		m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		// Don't set the advanced flags.
		m_swapChainDesc.Flags = 0;

		// Finally create the swap chain using the swap chain description and swap chain RPDC's command queue.
		auto l_commandQueue = reinterpret_cast<DX12CommandQueue*>(m_SwapChainRPDC->m_CommandQueue);

		IDXGISwapChain1* l_swapChain1;
		auto l_hResult = m_factory->CreateSwapChainForHwnd(
			l_commandQueue->m_CommandQueue,
			WinWindowSystemComponent::get().m_hwnd,
			&m_swapChainDesc,
			nullptr,
			nullptr,
			&l_swapChain1);

		l_hResult = l_swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));

		if (FAILED(l_hResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create swap chain!");
			m_objectStatus = ObjectStatus::Suspended;
			return false;
		}

		InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: Swap chain has been created.");

		// use device created swap chain textures
		for (size_t i = 0; i < m_swapChainImageCount; i++)
		{
			auto l_HResult = m_swapChain->GetBuffer((uint32_t)i, IID_PPV_ARGS(&m_swapChainImages[i]));
			if (FAILED(l_HResult))
			{
				InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't get pointer of swap chain image ", i, "!");
				return false;
			}
			auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(m_SwapChainRPDC->m_RenderTargets[i]);

			l_DX12TDC->m_ResourceHandle = m_swapChainImages[i];
			l_DX12TDC->m_DX12TextureDataDesc = l_DX12TDC->m_ResourceHandle->GetDesc();
			l_DX12TDC->m_WriteState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			l_DX12TDC->m_ReadState = D3D12_RESOURCE_STATE_PRESENT;
			l_DX12TDC->m_objectStatus = ObjectStatus::Activated;
		}

		// Initialize manually
		CreateViews(m_SwapChainRPDC, m_device);

		m_SwapChainRPDC->m_RenderTargetsResourceBinders.resize(m_swapChainImageCount);

		for (size_t i = 0; i < m_swapChainImageCount; i++)
		{
			m_SwapChainRPDC->m_RenderTargetsResourceBinders[i] = addResourcesBinder();
		}

		CreateResourcesBinder(m_SwapChainRPDC, this);

		CreateRootSignature(m_SwapChainRPDC, m_device);

		m_SwapChainRPDC->m_PipelineStateObject = addPSO();

		CreatePSO(m_SwapChainRPDC, m_device);

		m_SwapChainRPDC->m_Fences.resize(m_SwapChainRPDC->m_CommandLists.size());
		for (size_t i = 0; i < m_SwapChainRPDC->m_Fences.size(); i++)
		{
			m_SwapChainRPDC->m_Fences[i] = addFence();
		}

		CreateSyncPrimitives(m_SwapChainRPDC, m_device);

		m_SwapChainRPDC->m_objectStatus = ObjectStatus::Activated;
	}

	return true;
}

bool DX12RenderingServer::Terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer has been terminated.");

	return true;
}

ObjectStatus DX12RenderingServer::GetStatus()
{
	return m_objectStatus;
}

AddComponent(DX12, MeshData);
AddComponent(DX12, TextureData);
AddComponent(DX12, MaterialData);
AddComponent(DX12, RenderPassData);
AddComponent(DX12, ShaderProgram);
AddComponent(DX12, SamplerData);
AddComponent(DX12, GPUBufferData);

bool DX12RenderingServer::InitializeMeshDataComponent(MeshDataComponent * rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
	{
		return true;
	}

	// Flip y texture coordinate
	for (auto& i : rhs->m_vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}

	auto l_rhs = reinterpret_cast<DX12MeshDataComponent*>(rhs);

	// vertices
	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * l_rhs->m_vertices.size());

	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);
	l_rhs->m_vertexBuffer = CreateDefaultHeapBuffer(&l_verticesResourceDesc, m_device);

	if (l_rhs->m_vertexBuffer == nullptr)
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't create vertex buffer!");
		return false;
	}
#ifdef  _DEBUG
	SetObjectName(l_rhs, l_rhs->m_vertexBuffer, "VB");
#endif //  _DEBUG

	auto l_vertexUploadHeapBuffer = CreateUploadHeapBuffer(&l_verticesResourceDesc, m_device);

	auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);

	// main memory ----> upload heap
	D3D12_SUBRESOURCE_DATA l_verticesSubResourceData = {};
	l_verticesSubResourceData.pData = &l_rhs->m_vertices[0];
	l_verticesSubResourceData.RowPitch = l_verticesDataSize;
	l_verticesSubResourceData.SlicePitch = 1;
	UpdateSubresources(l_commandList, l_rhs->m_vertexBuffer, l_vertexUploadHeapBuffer, 0, 0, 1, &l_verticesSubResourceData);

	//  upload heap ----> default heap
	l_commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// Initialize the vertex buffer view.
	l_rhs->m_VBV.BufferLocation = l_rhs->m_vertexBuffer->GetGPUVirtualAddress();
	l_rhs->m_VBV.StrideInBytes = sizeof(Vertex);
	l_rhs->m_VBV.SizeInBytes = l_verticesDataSize;

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: VBO ", l_rhs->m_vertexBuffer, " is initialized.");

	// indices
	auto l_indicesDataSize = uint32_t(sizeof(Index) * l_rhs->m_indices.size());

	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);
	l_rhs->m_indexBuffer = CreateDefaultHeapBuffer(&l_indicesResourceDesc, m_device);

	if (l_rhs->m_indexBuffer == nullptr)
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't create index buffer!");
		return false;
	}
#ifdef  _DEBUG
	SetObjectName(l_rhs, l_rhs->m_indexBuffer, "IB");
#endif //  _DEBUG

	auto l_indexUploadHeapBuffer = CreateUploadHeapBuffer(&l_indicesResourceDesc, m_device);

	// main memory ----> upload heap
	D3D12_SUBRESOURCE_DATA l_indicesSubResourceData = {};
	l_indicesSubResourceData.pData = &l_rhs->m_indices[0];
	l_indicesSubResourceData.RowPitch = l_indicesDataSize;
	l_indicesSubResourceData.SlicePitch = 1;
	UpdateSubresources(l_commandList, l_rhs->m_indexBuffer, l_indexUploadHeapBuffer, 0, 0, 1, &l_indicesSubResourceData);

	//  upload heap ----> default heap
	l_commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);

	l_vertexUploadHeapBuffer->Release();
	l_indexUploadHeapBuffer->Release();

	// Initialize the index buffer view.
	l_rhs->m_IBV.Format = DXGI_FORMAT_R32_UINT;
	l_rhs->m_IBV.BufferLocation = l_rhs->m_indexBuffer->GetGPUVirtualAddress();
	l_rhs->m_IBV.SizeInBytes = l_indicesDataSize;

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: IBO ", l_rhs->m_indexBuffer, " is initialized.");

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool DX12RenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<DX12TextureDataComponent*>(rhs);

	l_rhs->m_DX12TextureDataDesc = GetDX12TextureDataDesc(l_rhs->m_textureDataDesc);
	l_rhs->m_PixelDataSize = GetTexturePixelDataSize(l_rhs->m_textureDataDesc);
	l_rhs->m_WriteState = GetTextureWriteState(l_rhs->m_textureDataDesc);
	l_rhs->m_ReadState = GetTextureReadState(l_rhs->m_textureDataDesc);

	if (l_rhs->m_textureDataDesc.CPUAccessibility != Accessibility::Immutable)
	{
		auto l_bufferSize = l_rhs->m_DX12TextureDataDesc.Width * l_rhs->m_DX12TextureDataDesc.Height * l_rhs->m_DX12TextureDataDesc.DepthOrArraySize * l_rhs->m_PixelDataSize;
		l_rhs->m_ResourceHandle = CreateReadBackHeapBuffer(l_bufferSize, m_device);
	}
	else
	{
		// Create the empty texture.
		if (l_rhs->m_textureDataDesc.UsageType == TextureUsageType::ColorAttachment
			|| l_rhs->m_textureDataDesc.UsageType == TextureUsageType::DepthAttachment
			|| l_rhs->m_textureDataDesc.UsageType == TextureUsageType::DepthStencilAttachment)
		{
			D3D12_CLEAR_VALUE l_clearValue;
			if (l_rhs->m_textureDataDesc.UsageType == TextureUsageType::ColorAttachment)
			{
				l_clearValue = D3D12_CLEAR_VALUE{ l_rhs->m_DX12TextureDataDesc.Format, { 0.0f, 0.0f, 0.0f, 0.0f } };
			}
			else if (l_rhs->m_textureDataDesc.UsageType == TextureUsageType::DepthAttachment)
			{
				l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
				l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
			}
			else if (l_rhs->m_textureDataDesc.UsageType == TextureUsageType::DepthStencilAttachment)
			{
				l_clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
			}
			l_rhs->m_ResourceHandle = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDataDesc, m_device, &l_clearValue);
		}
		else
		{
			l_rhs->m_ResourceHandle = CreateDefaultHeapBuffer(&l_rhs->m_DX12TextureDataDesc, m_device);
		}

		if (l_rhs->m_ResourceHandle == nullptr)
		{
			InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't create texture!");
			return false;
		}
#ifdef _DEBUG
		SetObjectName(l_rhs, l_rhs->m_ResourceHandle, "Texture");
#endif // _DEBUG

		std::vector<ID3D12Resource*> l_uploadBuffers;

		auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);

		// main memory ----> upload heap
		if (l_rhs->m_textureData)
		{
			if (l_rhs->m_textureDataDesc.SamplerType == TextureSamplerType::SamplerCubemap)
			{
				UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(l_rhs->m_ResourceHandle, 0, 6);

				for (uint32_t i = 0; i < 6; i++)
				{
					D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
					l_textureSubResourceData.RowPitch = l_rhs->m_textureDataDesc.Width * l_rhs->m_PixelDataSize;
					void* l_rawData = (unsigned char*)l_rhs->m_textureData + l_textureSubResourceData.RowPitch * l_rhs->m_textureDataDesc.Height * i;

					l_textureSubResourceData.pData = l_rawData;
					l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * l_rhs->m_textureDataDesc.Height;

					auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
					auto l_uploadHeapBuffer = CreateUploadHeapBuffer(&l_resourceDesc, m_device);
					UpdateSubresources(l_commandList, l_rhs->m_ResourceHandle, l_uploadHeapBuffer, 0, i, 1, &l_textureSubResourceData);
					l_uploadBuffers.emplace_back(l_uploadHeapBuffer);
				}
			}
			else
			{
				UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(l_rhs->m_ResourceHandle, 0, 1);
				D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
				l_textureSubResourceData.pData = l_rhs->m_textureData;
				l_textureSubResourceData.RowPitch = l_rhs->m_textureDataDesc.Width * l_rhs->m_PixelDataSize;
				l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * l_rhs->m_textureDataDesc.Height;

				auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_uploadHeapBufferSize);
				auto l_uploadHeapBuffer = CreateUploadHeapBuffer(&l_resourceDesc, m_device);
				UpdateSubresources(l_commandList, l_rhs->m_ResourceHandle, l_uploadHeapBuffer, 0, 0, 1, &l_textureSubResourceData);
				l_uploadBuffers.emplace_back(l_uploadHeapBuffer);
			}
		}

		//  upload heap ----> default heap
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_ResourceHandle, D3D12_RESOURCE_STATE_COPY_DEST, l_rhs->m_ReadState));
		EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);

		for (auto i : l_uploadBuffers)
		{
			i->Release();
		}

		auto l_resourceBinder = addResourcesBinder();
		l_resourceBinder->m_TextureSRV = CreateSRV(l_rhs);
		if (l_rhs->m_textureDataDesc.UsageType == TextureUsageType::RawImage)
		{
			l_resourceBinder->m_TextureUAV = CreateUAV(l_rhs);
		}

		l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Image;
		l_resourceBinder->m_Texture = l_rhs;

		l_rhs->m_ResourceBinder = l_resourceBinder;
	}

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: texture ", l_rhs, " is initialized.");

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	return true;
}

bool DX12RenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<DX12MaterialDataComponent*>(rhs);
	l_rhs->m_ResourceBinders.resize(5);

	if (l_rhs->m_normalTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_normalTexture);
		l_rhs->m_ResourceBinders[0] = l_rhs->m_normalTexture->m_ResourceBinder;
	}
	else
	{
		l_rhs->m_ResourceBinders[0] = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Normal)->m_ResourceBinder;
	}
	if (l_rhs->m_albedoTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_albedoTexture);
		l_rhs->m_ResourceBinders[1] = l_rhs->m_albedoTexture->m_ResourceBinder;
	}
	else
	{
		l_rhs->m_ResourceBinders[1] = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Albedo)->m_ResourceBinder;
	}
	if (l_rhs->m_metallicTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_metallicTexture);
		l_rhs->m_ResourceBinders[2] = l_rhs->m_metallicTexture->m_ResourceBinder;
	}
	else
	{
		l_rhs->m_ResourceBinders[2] = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Metallic)->m_ResourceBinder;
	}
	if (l_rhs->m_roughnessTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_roughnessTexture);
		l_rhs->m_ResourceBinders[3] = l_rhs->m_roughnessTexture->m_ResourceBinder;
	}
	else
	{
		l_rhs->m_ResourceBinders[3] = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Roughness)->m_ResourceBinder;
	}
	if (l_rhs->m_aoTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_aoTexture);
		l_rhs->m_ResourceBinders[4] = l_rhs->m_aoTexture->m_ResourceBinder;
	}
	else
	{
		l_rhs->m_ResourceBinders[4] = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::AmbientOcclusion)->m_ResourceBinder;
	}

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(l_rhs);

	return true;
}

bool DX12RenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);

	bool l_result = true;

	l_result &= ReserveRenderTargets(l_rhs, this);

	l_result &= CreateRenderTargets(l_rhs, this);

	l_result &= CreateViews(l_rhs, m_device);

	l_result &= CreateRootSignature(l_rhs, m_device);

	l_rhs->m_RenderTargetsResourceBinders.resize(l_rhs->m_RenderPassDesc.m_RenderTargetCount);
	for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		l_rhs->m_RenderTargetsResourceBinders[i] = addResourcesBinder();
	}

	l_result &= CreateResourcesBinder(l_rhs, this);

	l_rhs->m_PipelineStateObject = addPSO();

	l_result &= CreatePSO(l_rhs, m_device);

	l_rhs->m_CommandQueue = addCommandQueue();

	l_result &= CreateCommandQueue(l_rhs, m_device);
	l_result &= CreateCommandAllocators(l_rhs, m_device);

	l_rhs->m_CommandLists.resize(l_rhs->m_CommandAllocators.size());
	for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
	{
		l_rhs->m_CommandLists[i] = addCommandList();
	}

	l_result &= CreateCommandLists(l_rhs, m_device);

	l_rhs->m_Fences.resize(l_rhs->m_CommandLists.size());
	for (size_t i = 0; i < l_rhs->m_Fences.size(); i++)
	{
		l_rhs->m_Fences[i] = addFence();
	}

	l_result &= CreateSyncPrimitives(l_rhs, m_device);

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	return l_result;
}

bool DX12RenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent*>(rhs);

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

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeSamplerDataComponent(SamplerDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerDataComponent*>(rhs);
	auto l_resourceBinder = addResourcesBinder();

	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Sampler;

	l_resourceBinder->m_Sampler.SamplerDesc.Filter = GetFilterMode(l_rhs->m_SamplerDesc.m_MinFilterMethod, l_rhs->m_SamplerDesc.m_MagFilterMethod);
	l_resourceBinder->m_Sampler.SamplerDesc.AddressU = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodU);
	l_resourceBinder->m_Sampler.SamplerDesc.AddressV = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodV);
	l_resourceBinder->m_Sampler.SamplerDesc.AddressW = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodW);
	l_resourceBinder->m_Sampler.SamplerDesc.MipLODBias = 0.0f;
	l_resourceBinder->m_Sampler.SamplerDesc.MaxAnisotropy = l_rhs->m_SamplerDesc.m_MaxAnisotropy;
	l_resourceBinder->m_Sampler.SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	l_resourceBinder->m_Sampler.SamplerDesc.BorderColor[0] = l_rhs->m_SamplerDesc.m_BorderColor[0];
	l_resourceBinder->m_Sampler.SamplerDesc.BorderColor[1] = l_rhs->m_SamplerDesc.m_BorderColor[1];
	l_resourceBinder->m_Sampler.SamplerDesc.BorderColor[2] = l_rhs->m_SamplerDesc.m_BorderColor[2];
	l_resourceBinder->m_Sampler.SamplerDesc.BorderColor[3] = l_rhs->m_SamplerDesc.m_BorderColor[3];
	l_resourceBinder->m_Sampler.SamplerDesc.MinLOD = l_rhs->m_SamplerDesc.m_MinLOD;
	l_resourceBinder->m_Sampler.SamplerDesc.MaxLOD = l_rhs->m_SamplerDesc.m_MaxLOD;

	l_resourceBinder->m_Sampler.CPUHandle = m_currentSamplerCPUHandle;
	l_resourceBinder->m_Sampler.GPUHandle = m_currentSamplerGPUHandle;

	m_device->CreateSampler(&l_resourceBinder->m_Sampler.SamplerDesc, l_resourceBinder->m_Sampler.CPUHandle);

	auto l_samplerDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	m_currentSamplerCPUHandle.ptr += l_samplerDescSize;
	m_currentSamplerGPUHandle.ptr += l_samplerDescSize;

	l_rhs->m_ResourceBinder = l_resourceBinder;
	l_rhs->m_objectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferDataComponent*>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	auto l_resourceBinder = addResourcesBinder();
	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Buffer;
	l_resourceBinder->m_GPUAccessibility = l_rhs->m_GPUAccessibility;
	l_resourceBinder->m_ElementCount = l_rhs->m_ElementCount;
	l_resourceBinder->m_ElementSize = l_rhs->m_ElementSize;
	l_resourceBinder->m_TotalSize = l_rhs->m_TotalSize;

	auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize);
	l_rhs->m_UploadHeapResourceHandle = CreateUploadHeapBuffer(&l_resourceDesc, m_device);

#ifdef _DEBUG
	SetObjectName(rhs, l_rhs->m_UploadHeapResourceHandle, "UploadHeapGPUBuffer");
#endif // _DEBUG

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		if (l_rhs->m_CPUAccessibility == Accessibility::Immutable || l_rhs->m_CPUAccessibility == Accessibility::WriteOnly)
		{
			auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			l_rhs->m_DefaultHeapResourceHandle = CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc, m_device);

			auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);
			l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapResourceHandle, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);

#ifdef _DEBUG
			SetObjectName(rhs, l_rhs->m_DefaultHeapResourceHandle, "DefaultHeapGPUBuffer");
#endif // _DEBUG
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "DX12RenderingServer: Not support CPU-readable GPU buffer currently.");
		}
	}

	l_resourceBinder->m_DefaultHeapBuffer = l_rhs->m_DefaultHeapResourceHandle;
	l_resourceBinder->m_UploadHeapBuffer = l_rhs->m_UploadHeapResourceHandle;

	CD3DX12_RANGE m_readRange(0, 0);
	l_rhs->m_UploadHeapResourceHandle->Map(0, &m_readRange, &l_rhs->m_MappedMemory);

	if (l_rhs->m_InitialData)
	{
		UploadGPUBufferDataComponent(l_rhs, l_rhs->m_InitialData);
	}

	l_rhs->m_ResourceBinder = l_resourceBinder;

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12MeshDataComponent*>(rhs);

	l_rhs->m_vertexBuffer->Release();
	l_rhs->m_indexBuffer->Release();

	m_MeshDataComponentPool->Destroy(l_rhs);

	m_initializedMeshes.erase(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureDataComponent*>(rhs);

	l_rhs->m_ResourceHandle->Release();

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_TextureDataComponentPool->Destroy(l_rhs);

	m_initializedTextures.erase(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12MaterialDataComponent*>(rhs);

	m_MaterialDataComponentPool->Destroy(l_rhs);

	m_initializedMaterials.erase(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	l_PSO->m_PSO->Release();

	m_PSOPool->Destroy(l_PSO);

	if (l_rhs->m_DepthStencilRenderTarget)
	{
		DeleteTextureDataComponent(l_rhs->m_DepthStencilRenderTarget);
	}

	for (size_t i = 0; i < l_rhs->m_RenderTargets.size(); i++)
	{
		DeleteTextureDataComponent(l_rhs->m_RenderTargets[i]);
		m_ResourcesBinderPool->Destroy(l_rhs->m_RenderTargetsResourceBinders[i]);
	}

	m_RenderPassDataComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12ShaderProgramComponent*>(rhs);

	if (l_rhs->m_VSBuffer)
	{
		l_rhs->m_VSBuffer->Release();
	}
	if (l_rhs->m_HSBuffer)
	{
		l_rhs->m_HSBuffer->Release();
	}
	if (l_rhs->m_DSBuffer)
	{
		l_rhs->m_DSBuffer->Release();
	}
	if (l_rhs->m_GSBuffer)
	{
		l_rhs->m_GSBuffer->Release();
	}
	if (l_rhs->m_PSBuffer)
	{
		l_rhs->m_PSBuffer->Release();
	}
	if (l_rhs->m_CSBuffer)
	{
		l_rhs->m_CSBuffer->Release();
	}

	m_ShaderProgramComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::DeleteSamplerDataComponent(SamplerDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerDataComponent*>(rhs);

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_SamplerDataComponentPool->Destroy(l_rhs);

	return false;
}

bool DX12RenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferDataComponent*>(rhs);

	if (l_rhs->m_DefaultHeapResourceHandle)
	{
		l_rhs->m_DefaultHeapResourceHandle->Release();
	}

	if (l_rhs->m_UploadHeapResourceHandle)
	{
		l_rhs->m_UploadHeapResourceHandle->Release();
	}

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_GPUBufferDataComponentPool->Destroy(l_rhs);

	return true;
}

bool DX12RenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferDataComponent*>(rhs);

	auto l_size = l_rhs->m_TotalSize;
	if (range != SIZE_MAX)
	{
		l_size = range * l_rhs->m_ElementSize;
	}

	std::memcpy((char*)l_rhs->m_MappedMemory + startOffset * l_rhs->m_ElementSize, GPUBufferValue, l_size);

	if (l_rhs->m_DefaultHeapResourceHandle)
	{
		auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);

		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapResourceHandle, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
		l_commandList->CopyResource(l_rhs->m_DefaultHeapResourceHandle, l_rhs->m_UploadHeapResourceHandle);
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapResourceHandle, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);

		m_globalCommandAllocator->Reset();
	}

	return true;
}

bool DX12RenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[frameIndex]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	l_rhs->m_CurrentFrame = frameIndex;
	l_rhs->m_CommandAllocators[frameIndex]->Reset();

	l_commandList->m_GraphicsCommandList->Reset(l_rhs->m_CommandAllocators[frameIndex], l_PSO->m_PSO);

	return true;
}

bool PrepareRenderTargets(DX12RenderPassDataComponent* renderPass, DX12CommandList* commandList)
{
	if (renderPass->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		if (renderPass->m_RenderPassDesc.m_RenderTargetDesc.UsageType != TextureUsageType::RawImage)
		{
			if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
			{
				auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(renderPass->m_RenderTargets[renderPass->m_CurrentFrame]);
				commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, l_DX12TDC->m_ReadState, l_DX12TDC->m_WriteState));
			}
			else
			{
				for (size_t i = 0; i < renderPass->m_RenderPassDesc.m_RenderTargetCount; i++)
				{
					auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(renderPass->m_RenderTargets[i]);
					commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, l_DX12TDC->m_ReadState, l_DX12TDC->m_WriteState));
				}
			}
		}

		if (renderPass->m_DepthStencilRenderTarget)
		{
			auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(renderPass->m_DepthStencilRenderTarget);
			commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, l_DX12TDC->m_ReadState, l_DX12TDC->m_WriteState));
		}
	}

	return true;
}

bool PreparePipeline(DX12RenderPassDataComponent* renderPass, DX12CommandList* commandList, DX12PipelineStateObject* PSO)
{
	ID3D12DescriptorHeap* l_heaps[] = { m_CSUHeap, m_samplerHeap };

	commandList->m_GraphicsCommandList->SetDescriptorHeaps(2, l_heaps);
	commandList->m_GraphicsCommandList->SetPipelineState(PSO->m_PSO);

	if (renderPass->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		commandList->m_GraphicsCommandList->SetGraphicsRootSignature(renderPass->m_RootSignature);
		commandList->m_GraphicsCommandList->RSSetViewports(1, &PSO->m_Viewport);
		commandList->m_GraphicsCommandList->RSSetScissorRects(1, &PSO->m_Scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE* l_DSVDescriptorCPUHandle = NULL;

		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
		{
			l_DSVDescriptorCPUHandle = &renderPass->m_DSVDescriptorCPUHandle;
		}

		if (renderPass->m_RenderPassDesc.m_RenderTargetDesc.UsageType != TextureUsageType::RawImage)
		{
			if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
			{
				commandList->m_GraphicsCommandList->OMSetRenderTargets(1, &renderPass->m_RTVDescriptorCPUHandles[renderPass->m_CurrentFrame], FALSE, l_DSVDescriptorCPUHandle);
			}
			else
			{
				commandList->m_GraphicsCommandList->OMSetRenderTargets((uint32_t)renderPass->m_RenderPassDesc.m_RenderTargetCount, &renderPass->m_RTVDescriptorCPUHandles[0], FALSE, l_DSVDescriptorCPUHandle);
			}
		}

		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer)
		{
			commandList->m_GraphicsCommandList->OMSetStencilRef(renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference);
		}
	}
	else
	{
		commandList->m_GraphicsCommandList->SetComputeRootSignature(renderPass->m_RootSignature);
	}

	return true;
}

bool DX12RenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	PrepareRenderTargets(l_rhs, l_commandList);
	PreparePipeline(l_rhs, l_commandList, l_PSO);

	return true;
}

bool DX12RenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	if (rhs->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
		auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

		if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.UsageType != TextureUsageType::RawImage)
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				l_commandList->m_GraphicsCommandList->ClearRenderTargetView(l_rhs->m_RTVDescriptorCPUHandles[l_rhs->m_CurrentFrame], l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor, 0, nullptr);
			}
			else
			{
				for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
				{
					l_commandList->m_GraphicsCommandList->ClearRenderTargetView(l_rhs->m_RTVDescriptorCPUHandles[i], l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor, 0, nullptr);
				}
			}
		}
		else
		{
			// @TODO: SOOO verbose API DX12 it is!
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				auto l_RT = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame]);
				auto l_resourceBinder = reinterpret_cast<DX12ResourceBinder*>(l_RT->m_ResourceBinder);

				l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_resourceBinder->m_Texture->m_ResourceHandle, l_resourceBinder->m_Texture->m_ReadState, l_resourceBinder->m_Texture->m_WriteState));

				l_commandList->m_GraphicsCommandList->ClearUnorderedAccessViewFloat(
					l_resourceBinder->m_TextureUAV.ShaderNonVisibleGPUHandle,
					l_resourceBinder->m_TextureUAV.ShaderNonVisibleCPUHandle,
					l_RT->m_ResourceHandle,
					l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor,
					0,
					NULL);

				l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_resourceBinder->m_Texture->m_ResourceHandle, l_resourceBinder->m_Texture->m_WriteState, l_resourceBinder->m_Texture->m_ReadState));
			}
			else
			{
				for (auto i : l_rhs->m_RenderTargets)
				{
					auto l_RT = reinterpret_cast<DX12TextureDataComponent*>(i);
					auto l_resourceBinder = reinterpret_cast<DX12ResourceBinder*>(l_RT->m_ResourceBinder);

					l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_resourceBinder->m_Texture->m_ResourceHandle, l_resourceBinder->m_Texture->m_ReadState, l_resourceBinder->m_Texture->m_WriteState));

					l_commandList->m_GraphicsCommandList->ClearUnorderedAccessViewFloat(
						l_resourceBinder->m_TextureUAV.ShaderNonVisibleGPUHandle,
						l_resourceBinder->m_TextureUAV.ShaderNonVisibleCPUHandle,
						l_RT->m_ResourceHandle,
						l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor,
						0,
						NULL);

					l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_resourceBinder->m_Texture->m_ResourceHandle, l_resourceBinder->m_Texture->m_WriteState, l_resourceBinder->m_Texture->m_ReadState));
				}
			}
		}

		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
		{
			l_commandList->m_GraphicsCommandList->ClearDepthStencilView(l_rhs->m_DSVDescriptorCPUHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x00, 0, nullptr);
		}
	}

	return true;
}

bool DX12RenderingServer::ActivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	auto l_resourceBinder = reinterpret_cast<DX12ResourceBinder*>(binder);
	auto l_renderPass = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if (l_resourceBinder)
	{
		if (shaderStage == ShaderStage::Compute)
		{
			switch (l_resourceBinder->m_ResourceBinderType)
			{
			case ResourceBinderType::Sampler:
				l_commandList->m_GraphicsCommandList->SetComputeRootDescriptorTable((uint32_t)globalSlot, l_resourceBinder->m_Sampler.GPUHandle);
				break;
			case ResourceBinderType::Image:
				if (accessibility != Accessibility::ReadOnly)
				{
					l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_resourceBinder->m_Texture->m_ResourceHandle, l_resourceBinder->m_Texture->m_ReadState, l_resourceBinder->m_Texture->m_WriteState));
					l_commandList->m_GraphicsCommandList->SetComputeRootDescriptorTable((uint32_t)globalSlot, l_resourceBinder->m_TextureUAV.ShaderVisibleGPUHandle);
				}
				else
				{
					l_commandList->m_GraphicsCommandList->SetComputeRootDescriptorTable((uint32_t)globalSlot, l_resourceBinder->m_TextureSRV.GPUHandle);
				}
				break;
			case ResourceBinderType::Buffer:
				if (l_resourceBinder->m_GPUAccessibility == Accessibility::ReadOnly)
				{
					if (accessibility != Accessibility::ReadOnly)
					{
						InnoLogger::Log(LogLevel::Warning, "DX12RenderingServer: Not allow GPU write to Constant Buffer!");
					}
					else
					{
						l_commandList->m_GraphicsCommandList->SetComputeRootConstantBufferView((uint32_t)globalSlot, l_resourceBinder->m_UploadHeapBuffer->GetGPUVirtualAddress() + startOffset * l_resourceBinder->m_ElementSize);
					}
				}
				else
				{
					if (accessibility != Accessibility::ReadOnly)
					{
						l_commandList->m_GraphicsCommandList->SetComputeRootUnorderedAccessView((uint32_t)globalSlot, l_resourceBinder->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * l_resourceBinder->m_ElementSize);
					}
					else
					{
						l_commandList->m_GraphicsCommandList->SetComputeRootShaderResourceView((uint32_t)globalSlot, l_resourceBinder->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * l_resourceBinder->m_ElementSize);
					}
				}

				break;
			default:
				break;
			}
		}
		else
		{
			switch (l_resourceBinder->m_ResourceBinderType)
			{
			case ResourceBinderType::Sampler:
				l_commandList->m_GraphicsCommandList->SetGraphicsRootDescriptorTable((uint32_t)globalSlot, l_resourceBinder->m_Sampler.GPUHandle);
				break;
			case ResourceBinderType::Image:
				if (accessibility != Accessibility::ReadOnly)
				{
					l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_resourceBinder->m_Texture->m_ResourceHandle, l_resourceBinder->m_Texture->m_ReadState, l_resourceBinder->m_Texture->m_WriteState));
					l_commandList->m_GraphicsCommandList->SetGraphicsRootDescriptorTable((uint32_t)globalSlot, l_resourceBinder->m_TextureUAV.ShaderVisibleGPUHandle);
				}
				else
				{
					l_commandList->m_GraphicsCommandList->SetGraphicsRootDescriptorTable((uint32_t)globalSlot, l_resourceBinder->m_TextureSRV.GPUHandle);
				}
				break;
			case ResourceBinderType::Buffer:
				if (l_resourceBinder->m_GPUAccessibility == Accessibility::ReadOnly)
				{
					if (accessibility != Accessibility::ReadOnly)
					{
						InnoLogger::Log(LogLevel::Warning, "DX12RenderingServer: Not allow GPU write to Constant Buffer!");
					}
					else
					{
						l_commandList->m_GraphicsCommandList->SetGraphicsRootConstantBufferView((uint32_t)globalSlot, l_resourceBinder->m_UploadHeapBuffer->GetGPUVirtualAddress() + startOffset * l_resourceBinder->m_ElementSize);
					}
				}
				else
				{
					if (accessibility != Accessibility::ReadOnly)
					{
						l_commandList->m_GraphicsCommandList->SetGraphicsRootUnorderedAccessView((uint32_t)globalSlot, l_resourceBinder->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * l_resourceBinder->m_ElementSize);
					}
					else
					{
						l_commandList->m_GraphicsCommandList->SetGraphicsRootShaderResourceView((uint32_t)globalSlot, l_resourceBinder->m_DefaultHeapBuffer->GetGPUVirtualAddress() + startOffset * l_resourceBinder->m_ElementSize);
					}
				}
				break;
			default:
				break;
			}
		}
	}

	return true;
}

bool DX12RenderingServer::DispatchDrawCall(RenderPassDataComponent * renderPass, MeshDataComponent* mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<DX12MeshDataComponent*>(mesh);

	l_commandList->m_GraphicsCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_GraphicsCommandList->IASetVertexBuffers(0, 1, &l_mesh->m_VBV);
	l_commandList->m_GraphicsCommandList->IASetIndexBuffer(&l_mesh->m_IBV);
	l_commandList->m_GraphicsCommandList->DrawIndexedInstanced((uint32_t)l_mesh->m_indicesSize, (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12RenderingServer::DeactivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	auto l_resourceBinder = reinterpret_cast<DX12ResourceBinder*>(binder);
	auto l_renderPass = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if (l_resourceBinder)
	{
		if (l_resourceBinder->m_ResourceBinderType == ResourceBinderType::Image)
		{
			if (accessibility != Accessibility::ReadOnly)
			{
				l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_resourceBinder->m_Texture->m_ResourceHandle, l_resourceBinder->m_Texture->m_WriteState, l_resourceBinder->m_Texture->m_ReadState));
			}
		}
	}

	return true;
}

bool DX12RenderingServer::CommandListEnd(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.UsageType != TextureUsageType::RawImage)
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame]);
				l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, l_DX12TDC->m_WriteState, l_DX12TDC->m_ReadState));
			}
			else
			{
				for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
				{
					auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_RenderTargets[i]);
					l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, l_DX12TDC->m_WriteState, l_DX12TDC->m_ReadState));
				}
			}
		}

		if (l_rhs->m_DepthStencilRenderTarget)
		{
			auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_DepthStencilRenderTarget);
			l_commandList->m_GraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, l_DX12TDC->m_WriteState, l_DX12TDC->m_ReadState));
		}
	}

	l_commandList->m_GraphicsCommandList->Close();

	return true;
}

bool DX12RenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandQueue = reinterpret_cast<DX12CommandQueue*>(l_rhs->m_CommandQueue);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_fence = reinterpret_cast<DX12Fence*>(l_rhs->m_Fences[l_rhs->m_CurrentFrame]);

	ID3D12CommandList* l_commandLists[] = { l_commandList->m_GraphicsCommandList };

	l_commandQueue->m_CommandQueue->ExecuteCommandLists(1, l_commandLists);

	const UINT64 l_finishedFenceValue = l_fence->m_FenceStatus + 1;
	l_commandQueue->m_CommandQueue->Signal(l_fence->m_Fence, l_finishedFenceValue);

	return true;
}

bool DX12RenderingServer::WaitForFrame(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_fence = reinterpret_cast<DX12Fence*>(l_rhs->m_Fences[l_rhs->m_CurrentFrame]);

	const UINT64 l_currentFenceValue = l_fence->m_Fence->GetCompletedValue();
	const UINT64 l_expectedFenceValue = l_fence->m_FenceStatus + 1;

	if (l_currentFenceValue < l_expectedFenceValue)
	{
		l_fence->m_Fence->SetEventOnCompletion(l_expectedFenceValue, l_fence->m_FenceEvent);
		WaitForSingleObjectEx(l_fence->m_FenceEvent, INFINITE, FALSE);
	}
	l_fence->m_FenceStatus = l_expectedFenceValue;

	return true;
}

bool DX12RenderingServer::SetUserPipelineOutput(RenderPassDataComponent * rhs)
{
	m_userPipelineOutput = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);

	return true;
}

bool DX12RenderingServer::Present()
{
	CommandListBegin(m_SwapChainRPDC, m_SwapChainRPDC->m_CurrentFrame);

	auto l_commandList = reinterpret_cast<DX12CommandList*>(m_SwapChainRPDC->m_CommandLists[m_SwapChainRPDC->m_CurrentFrame]);
	auto l_commandQueue = reinterpret_cast<DX12CommandQueue*>(m_SwapChainRPDC->m_CommandQueue);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(m_SwapChainRPDC->m_PipelineStateObject);

	PrepareRenderTargets(m_SwapChainRPDC, l_commandList);

	PreparePipeline(m_SwapChainRPDC, l_commandList, l_PSO);

	CleanRenderTargets(m_SwapChainRPDC);

	ActivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_SwapChainSDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	ActivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_userPipelineOutput->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Quad);

	DispatchDrawCall(m_SwapChainRPDC, l_mesh, 1);

	DeactivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_userPipelineOutput->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	CommandListEnd(m_SwapChainRPDC);

	ID3D12CommandList* l_commandLists[] = { l_commandList->m_GraphicsCommandList };

	l_commandQueue->m_CommandQueue->ExecuteCommandLists(1, l_commandLists);

	// Present the frame.
	m_swapChain->Present(1, 0);

	// Schedule a Signal command in the queue.
	auto l_currentFence = reinterpret_cast<DX12Fence*>(m_SwapChainRPDC->m_Fences[m_SwapChainRPDC->m_CurrentFrame]);
	const UINT64 l_currentFenceValue = l_currentFence->m_FenceStatus;

	l_commandQueue->m_CommandQueue->Signal(l_currentFence->m_Fence, l_currentFenceValue);

	auto l_nextFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (l_currentFence->m_Fence->GetCompletedValue() < l_currentFenceValue)
	{
		l_currentFence->m_Fence->SetEventOnCompletion(l_currentFenceValue, l_currentFence->m_FenceEvent);
		WaitForSingleObjectEx(l_currentFence->m_FenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	auto l_nextFence = reinterpret_cast<DX12Fence*>(m_SwapChainRPDC->m_Fences[l_nextFrameIndex]);
	l_nextFence->m_FenceStatus = l_currentFenceValue + 1;

	// Update the frame index.
	m_SwapChainRPDC->m_CurrentFrame = l_nextFrameIndex;

	return true;
}

bool DX12RenderingServer::DispatchCompute(RenderPassDataComponent * renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	l_commandList->m_GraphicsCommandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

bool DX12RenderingServer::CopyDepthStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(dest->m_CommandLists[dest->m_CurrentFrame]);

	auto l_src = reinterpret_cast<DX12TextureDataComponent*>(src->m_DepthStencilRenderTarget);
	auto l_dest = reinterpret_cast<DX12TextureDataComponent*>(dest->m_DepthStencilRenderTarget);

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_ResourceHandle,
			l_src->m_ReadState,
			D3D12_RESOURCE_STATE_COPY_SOURCE));

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_ResourceHandle,
			l_dest->m_WriteState,
			D3D12_RESOURCE_STATE_COPY_DEST));

	l_commandList->m_GraphicsCommandList->CopyResource(l_dest->m_ResourceHandle, l_src->m_ResourceHandle);

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_ResourceHandle,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			l_src->m_ReadState));

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_ResourceHandle,
			D3D12_RESOURCE_STATE_COPY_DEST,
			l_dest->m_WriteState));

	return true;
}

bool DX12RenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(dest->m_CommandLists[dest->m_CurrentFrame]);

	auto l_src = reinterpret_cast<DX12TextureDataComponent*>(src->m_RenderTargets[srcIndex]);
	auto l_dest = reinterpret_cast<DX12TextureDataComponent*>(dest->m_RenderTargets[destIndex]);

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_ResourceHandle,
			l_src->m_ReadState,
			D3D12_RESOURCE_STATE_COPY_SOURCE));

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_ResourceHandle,
			l_dest->m_WriteState,
			D3D12_RESOURCE_STATE_COPY_DEST));

	l_commandList->m_GraphicsCommandList->CopyResource(l_dest->m_ResourceHandle, l_src->m_ResourceHandle);

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_ResourceHandle,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			l_src->m_ReadState));

	l_commandList->m_GraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_ResourceHandle,
			D3D12_RESOURCE_STATE_COPY_DEST,
			l_dest->m_WriteState));

	return true;
}

Vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	// @TODO: Support different pixel data type

	auto l_srcTDC = reinterpret_cast<DX12TextureDataComponent*>(TDC);

	size_t l_sampleCount;

	switch (l_srcTDC->m_textureDataDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		l_sampleCount = l_srcTDC->m_textureDataDesc.Width;
		break;
	case TextureSamplerType::Sampler2D:
		l_sampleCount = l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_textureDataDesc.Height;
		break;
	case TextureSamplerType::Sampler3D:
		l_sampleCount = l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_textureDataDesc.Height * l_srcTDC->m_textureDataDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler1DArray:
		l_sampleCount = l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_textureDataDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::Sampler2DArray:
		l_sampleCount = l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_textureDataDesc.Height * l_srcTDC->m_textureDataDesc.DepthOrArraySize;
		break;
	case TextureSamplerType::SamplerCubemap:
		l_sampleCount = l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_textureDataDesc.Height * 6;
		break;
	default:
		break;
	}

	std::vector<Vec4> l_result;
	l_result.resize(l_sampleCount);

	auto l_destTDC = reinterpret_cast<DX12TextureDataComponent*>(AddTextureDataComponent("ReadBackTemp/"));
	l_destTDC->m_textureDataDesc = TDC->m_textureDataDesc;
	l_destTDC->m_textureDataDesc.CPUAccessibility = Accessibility::ReadOnly;

	InitializeTextureDataComponent(l_destTDC);

	if (l_srcTDC->m_textureDataDesc.SamplerType == TextureSamplerType::SamplerCubemap)
	{
		for (uint32_t i = 0; i < 6; i++)
		{
			D3D12_TEXTURE_COPY_LOCATION l_srcLocation;
			l_srcLocation.pResource = l_srcTDC->m_ResourceHandle;
			l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			l_srcLocation.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION l_destLocation;
			l_destLocation.pResource = l_destTDC->m_ResourceHandle;
			l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			l_destLocation.PlacedFootprint.Offset = i * l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_textureDataDesc.Height * l_srcTDC->m_PixelDataSize;
			l_destLocation.PlacedFootprint.Footprint.Format = l_srcTDC->m_DX12TextureDataDesc.Format;
			l_destLocation.PlacedFootprint.Footprint.Width = (uint32_t)l_srcTDC->m_DX12TextureDataDesc.Width;
			l_destLocation.PlacedFootprint.Footprint.Height = (uint32_t)l_srcTDC->m_DX12TextureDataDesc.Height;
			l_destLocation.PlacedFootprint.Footprint.Depth = 1;
			l_destLocation.PlacedFootprint.Footprint.RowPitch = (uint32_t)l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_PixelDataSize;

			auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);

			l_commandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);

			EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);
		}
	}
	else
	{
		D3D12_TEXTURE_COPY_LOCATION l_srcLocation;
		l_srcLocation.pResource = l_srcTDC->m_ResourceHandle;
		l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		l_srcLocation.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION l_destLocation;
		l_destLocation.pResource = l_destTDC->m_ResourceHandle;
		l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		l_destLocation.PlacedFootprint.Offset = 0;
		l_destLocation.PlacedFootprint.Footprint.Format = l_srcTDC->m_DX12TextureDataDesc.Format;
		l_destLocation.PlacedFootprint.Footprint.Width = (uint32_t)l_srcTDC->m_DX12TextureDataDesc.Width;
		l_destLocation.PlacedFootprint.Footprint.Height = (uint32_t)l_srcTDC->m_DX12TextureDataDesc.Height;
		l_destLocation.PlacedFootprint.Footprint.Depth = (uint32_t)l_srcTDC->m_DX12TextureDataDesc.DepthOrArraySize;
		l_destLocation.PlacedFootprint.Footprint.RowPitch = (uint32_t)l_srcTDC->m_textureDataDesc.Width * l_srcTDC->m_PixelDataSize;

		auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);

		l_commandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);

		EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);
	}

	CD3DX12_RANGE m_ReadRange(0, l_sampleCount * l_srcTDC->m_PixelDataSize);
	void* l_pData;
	auto l_HResult = l_destTDC->m_ResourceHandle->Map(0, &m_ReadRange, &l_pData);

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't map texture for CPU to read!");
	}

	std::memcpy(l_result.data(), l_pData, l_sampleCount * l_srcTDC->m_PixelDataSize);
	l_destTDC->m_ResourceHandle->Unmap(0, 0);

	DeleteTextureDataComponent(l_destTDC);

	return l_result;
}

bool DX12RenderingServer::Resize()
{
	return true;
}

DX12SRV DX12RenderingServer::CreateSRV(TextureDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureDataComponent*>(rhs);

	DX12SRV l_result = {};

	l_result.SRVDesc = GetSRVDesc(l_rhs->m_textureDataDesc, l_rhs->m_DX12TextureDataDesc);

	l_result.CPUHandle = m_currentCSUCPUHandle;
	l_result.GPUHandle = m_currentCSUGPUHandle;

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_currentCSUCPUHandle.ptr += l_CSUDescSize;
	m_currentCSUGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateShaderResourceView(l_rhs->m_ResourceHandle, &l_result.SRVDesc, l_result.CPUHandle);

	return l_result;
}

DX12UAV DX12RenderingServer::CreateUAV(TextureDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureDataComponent*>(rhs);

	DX12UAV l_result = {};

	l_result.UAVDesc = GetUAVDesc(l_rhs->m_textureDataDesc, l_rhs->m_DX12TextureDataDesc);

	l_result.ShaderNonVisibleCPUHandle = m_currentShaderNonVisibleCSUCPUHandle;
	l_result.ShaderNonVisibleGPUHandle = m_currentShaderNonVisibleCSUGPUHandle;
	l_result.ShaderVisibleCPUHandle = m_currentCSUCPUHandle;
	l_result.ShaderVisibleGPUHandle = m_currentCSUGPUHandle;

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_currentCSUCPUHandle.ptr += l_CSUDescSize;
	m_currentCSUGPUHandle.ptr += l_CSUDescSize;
	m_currentShaderNonVisibleCSUCPUHandle.ptr += l_CSUDescSize;
	m_currentShaderNonVisibleCSUGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateUnorderedAccessView(l_rhs->m_ResourceHandle, 0, &l_result.UAVDesc, l_result.ShaderNonVisibleCPUHandle);
	m_device->CreateUnorderedAccessView(l_rhs->m_ResourceHandle, 0, &l_result.UAVDesc, l_result.ShaderVisibleCPUHandle);

	return l_result;
}

DX12CBV DX12RenderingServer::CreateCBV(GPUBufferDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferDataComponent*>(rhs);

	DX12CBV l_result;

	l_result.CBVDesc.BufferLocation = l_rhs->m_UploadHeapResourceHandle->GetGPUVirtualAddress();
	l_result.CBVDesc.SizeInBytes = (uint32_t)l_rhs->m_ElementSize;

	l_result.CPUHandle = m_currentCSUCPUHandle;
	l_result.GPUHandle = m_currentCSUGPUHandle;

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_currentCSUCPUHandle.ptr += l_CSUDescSize;
	m_currentCSUGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateConstantBufferView(&l_result.CBVDesc, l_result.CPUHandle);

	return l_result;
}
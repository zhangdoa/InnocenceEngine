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

	TVec2<unsigned int> m_refreshRate = TVec2<unsigned int>(0, 1);

	int m_videoCardMemory = 0;
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
	const unsigned int m_swapChainImageCount = 2;
	std::vector<ID3D12Resource*> m_swapChainImages(m_swapChainImageCount);

	ID3D12DescriptorHeap* m_CSUHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC m_CSUHeapDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_initialCSUCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_initialCSUGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentCSUCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_currentCSUGPUHandle;

	ID3D12DescriptorHeap* m_samplerHeap = 0;
	D3D12_DESCRIPTOR_HEAP_DESC m_samplerHeapDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_initialSamplerCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_initialSamplerGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentSamplerCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_currentSamplerGPUHandle;

	DX12RenderPassDataComponent* m_SwapChainRPDC = 0;
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
	m_debugInterface->SetEnableGPUBasedValidation(true);

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

	unsigned int l_numModes;
	unsigned long long l_stringLength;

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

	for (unsigned int i = 0; i < l_numModes; i++)
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
	m_videoCardMemory = (int)(m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

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
	m_CSUHeapDesc.NumDescriptors = 65536;
	m_CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = m_device->CreateDescriptorHeap(&m_CSUHeapDesc, IID_PPV_ARGS(&m_CSUHeap));
	if (FAILED(l_result))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create DescriptorHeap for CBV/SRV/UAV!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_CSUHeap->SetName(L"GlobalCSUHeap");

	m_initialCSUCPUHandle = m_CSUHeap->GetCPUDescriptorHandleForHeapStart();
	m_initialCSUGPUHandle = m_CSUHeap->GetGPUDescriptorHandleForHeapStart();

	m_currentCSUCPUHandle = m_initialCSUCPUHandle;
	m_currentCSUGPUHandle = m_initialCSUGPUHandle;

	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer: DescriptorHeap for CBV/SRV/UAV has been created.");

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

	l_result &= CreateDebugCallback();
	l_result &= CreatePhysicalDevices();
	l_result &= CreateGlobalCommandQueue();
	l_result &= CreateGlobalCommandAllocator();
	l_result &= CreateGlobalCSUHeap();
	l_result &= CreateGlobalSamplerHeap();

	m_SwapChainRPDC = reinterpret_cast<DX12RenderPassDataComponent*>(AddRenderPassDataComponent("SwapChain/"));

	m_objectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "DX12RenderingServer setup finished.");

	return l_result;
}

bool DX12RenderingServer::Initialize()
{
	if (m_objectStatus == ObjectStatus::Created)
	{
		// Create command queue first
		auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

		l_RenderPassDesc.m_RenderTargetCount = m_swapChainImageCount;

		m_SwapChainRPDC->m_RenderPassDesc = l_RenderPassDesc;
		m_SwapChainRPDC->m_RenderPassDesc.m_UseMultiFrames = true;
		m_SwapChainRPDC->m_RenderPassDesc.m_RenderTargetDesc.pixelDataType = TexturePixelDataType::UBYTE;

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
			auto l_HResult = m_swapChain->GetBuffer((unsigned int)i, IID_PPV_ARGS(&m_swapChainImages[i]));
			if (FAILED(l_HResult))
			{
				InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't get pointer of swap chain image ", i, "!");
				return false;
			}
			auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(m_SwapChainRPDC->m_RenderTargets[i]);

			l_DX12TDC->m_ResourceHandle = m_swapChainImages[i];
			l_DX12TDC->m_DX12TextureDataDesc = l_DX12TDC->m_ResourceHandle->GetDesc();
			l_DX12TDC->m_objectStatus = ObjectStatus::Activated;
		}

		// Initialize manually
		CreateViews(m_SwapChainRPDC, m_device);

		m_SwapChainRPDC->m_RenderTargetsResourceBinder = addResourcesBinder();

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

MeshDataComponent * DX12RenderingServer::AddMeshDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_MeshDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)DX12MeshDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("Mesh_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

TextureDataComponent * DX12RenderingServer::AddTextureDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_TextureDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)DX12TextureDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("Texture_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

MaterialDataComponent * DX12RenderingServer::AddMaterialDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_MaterialDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)DX12MaterialDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("Material_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

RenderPassDataComponent * DX12RenderingServer::AddRenderPassDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_RenderPassDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)DX12RenderPassDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("RenderPass_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

ShaderProgramComponent * DX12RenderingServer::AddShaderProgramComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_ShaderProgramComponentPool->Spawn();
	auto l_result = new(l_rawPtr)DX12ShaderProgramComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("ShaderProgram_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

SamplerDataComponent * DX12RenderingServer::AddSamplerDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_SamplerDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)DX12SamplerDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("SamplerData_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

GPUBufferDataComponent * DX12RenderingServer::AddGPUBufferDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_GPUBufferDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)DX12GPUBufferDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("GPUBufferData_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

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
	auto l_verticesDataSize = unsigned int(sizeof(Vertex) * l_rhs->m_vertices.size());

	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);
	l_rhs->m_vertexBuffer = CreateDefaultHeapBuffer(&l_verticesResourceDesc, m_device);

	if (l_rhs->m_vertexBuffer == nullptr)
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't create vertex buffer!");
		return false;
	}

	auto l_uploadHeapBuffer = CreateUploadHeapBuffer(l_verticesDataSize, m_device);

	auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);

	// main memory ----> upload heap
	D3D12_SUBRESOURCE_DATA l_verticesSubResourceData = {};
	l_verticesSubResourceData.pData = &l_rhs->m_vertices[0];
	l_verticesSubResourceData.RowPitch = l_verticesDataSize;
	l_verticesSubResourceData.SlicePitch = 1;
	UpdateSubresources(l_commandList, l_rhs->m_vertexBuffer, l_uploadHeapBuffer, 0, 0, 1, &l_verticesSubResourceData);

	//  upload heap ----> default heap
	l_commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// Initialize the vertex buffer view.
	l_rhs->m_VBV.BufferLocation = l_rhs->m_vertexBuffer->GetGPUVirtualAddress();
	l_rhs->m_VBV.StrideInBytes = sizeof(Vertex);
	l_rhs->m_VBV.SizeInBytes = l_verticesDataSize;

#ifdef  _DEBUG
	SetObjectName(l_rhs, l_rhs->m_vertexBuffer, "VB");
#endif //  _DEBUG

	InnoLogger::Log(LogLevel::Verbose, "DX12RenderingServer: VBO ", l_rhs->m_vertexBuffer, " is initialized.");

	// indices
	auto l_indicesDataSize = unsigned int(sizeof(Index) * l_rhs->m_indices.size());

	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);
	l_rhs->m_indexBuffer = CreateDefaultHeapBuffer(&l_indicesResourceDesc, m_device);

	if (l_rhs->m_indexBuffer == nullptr)
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: can't create index buffer!");
		return false;
	}

	l_uploadHeapBuffer = CreateUploadHeapBuffer(l_indicesDataSize, m_device);

	// main memory ----> upload heap
	D3D12_SUBRESOURCE_DATA l_indicesSubResourceData = {};
	l_indicesSubResourceData.pData = &l_rhs->m_indices[0];
	l_indicesSubResourceData.RowPitch = l_indicesDataSize;
	l_indicesSubResourceData.SlicePitch = 1;
	UpdateSubresources(l_commandList, l_rhs->m_indexBuffer, l_uploadHeapBuffer, 0, 0, 1, &l_indicesSubResourceData);

	//  upload heap ----> default heap
	l_commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);

	// Initialize the index buffer view.
	l_rhs->m_IBV.Format = DXGI_FORMAT_R32_UINT;
	l_rhs->m_IBV.BufferLocation = l_rhs->m_indexBuffer->GetGPUVirtualAddress();
	l_rhs->m_IBV.SizeInBytes = l_indicesDataSize;

#ifdef  _DEBUG
	SetObjectName(l_rhs, l_rhs->m_indexBuffer, "IB");
#endif //  _DEBUG

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

	// Create the empty texture.
	if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		D3D12_CLEAR_VALUE l_clearValue;
		if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
		{
			l_clearValue = D3D12_CLEAR_VALUE{ l_rhs->m_DX12TextureDataDesc.Format, { 0.0f, 0.0f, 0.0f, 0.0f } };
		}
		else if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
		{
			l_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
			l_clearValue.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0x00 };
		}
		else if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
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

	auto l_commandList = BeginSingleTimeCommands(m_device, m_globalCommandAllocator);

	// main memory ----> upload heap
	if (!(l_rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE))
	{
		const UINT64 l_uploadHeapBufferSize = GetRequiredIntermediateSize(l_rhs->m_ResourceHandle, 0, 1);

		auto l_uploadHeapBuffer = CreateUploadHeapBuffer(l_uploadHeapBufferSize, m_device);

		D3D12_SUBRESOURCE_DATA l_textureSubResourceData = {};
		l_textureSubResourceData.pData = l_rhs->m_textureData;
		l_textureSubResourceData.RowPitch = l_rhs->m_textureDataDesc.width * ((unsigned int)l_rhs->m_textureDataDesc.pixelDataFormat + 1);
		l_textureSubResourceData.SlicePitch = l_textureSubResourceData.RowPitch * l_rhs->m_textureDataDesc.height;
		UpdateSubresources(l_commandList, l_rhs->m_ResourceHandle, l_uploadHeapBuffer, 0, 0, 1, &l_textureSubResourceData);
	}

	//  upload heap ----> default heap
	if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		l_commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_ResourceHandle, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
	else if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_ResourceHandle, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	}
	else if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		l_commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_ResourceHandle, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
	else
	{
		l_commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_ResourceHandle, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	EndSingleTimeCommands(l_commandList, m_device, m_globalCommandQueue);

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

	l_rhs->m_SRVs.resize(5);

	if (l_rhs->m_normalTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_normalTexture);
		l_rhs->m_SRVs[0] = CreateSRV(l_rhs->m_normalTexture);
	}
	else
	{
		l_rhs->m_SRVs[0] = CreateSRV(g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::NORMAL));
	}
	if (l_rhs->m_albedoTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_albedoTexture);
		l_rhs->m_SRVs[1] = CreateSRV(l_rhs->m_albedoTexture);
	}
	else
	{
		l_rhs->m_SRVs[1] = CreateSRV(g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::ALBEDO));
	}
	if (l_rhs->m_metallicTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_metallicTexture);
		l_rhs->m_SRVs[2] = CreateSRV(l_rhs->m_metallicTexture);
	}
	else
	{
		l_rhs->m_SRVs[2] = CreateSRV(g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::METALLIC));
	}
	if (l_rhs->m_roughnessTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_roughnessTexture);
		l_rhs->m_SRVs[3] = CreateSRV(l_rhs->m_roughnessTexture);
	}
	else
	{
		l_rhs->m_SRVs[3] = CreateSRV(g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	}
	if (l_rhs->m_aoTexture)
	{
		InitializeTextureDataComponent(l_rhs->m_aoTexture);
		l_rhs->m_SRVs[4] = CreateSRV(l_rhs->m_aoTexture);
	}
	else
	{
		l_rhs->m_SRVs[4] = CreateSRV(g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));
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

	l_rhs->m_RenderTargetsResourceBinder = addResourcesBinder();

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
		LoadShaderFile(&l_rhs->m_VSBuffer, ShaderType::VERTEX, l_rhs->m_ShaderFilePaths.m_VSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_TCSPath != "")
	{
		LoadShaderFile(&l_rhs->m_TCSBuffer, ShaderType::TCS, l_rhs->m_ShaderFilePaths.m_TCSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_TESPath != "")
	{
		LoadShaderFile(&l_rhs->m_TESBuffer, ShaderType::TES, l_rhs->m_ShaderFilePaths.m_TESPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(&l_rhs->m_GSBuffer, ShaderType::GEOMETRY, l_rhs->m_ShaderFilePaths.m_GSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_FSPath != "")
	{
		LoadShaderFile(&l_rhs->m_FSBuffer, ShaderType::FRAGMENT, l_rhs->m_ShaderFilePaths.m_FSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(&l_rhs->m_CSBuffer, ShaderType::COMPUTE, l_rhs->m_ShaderFilePaths.m_CSPath);
	}

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::InitializeSamplerDataComponent(SamplerDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12SamplerDataComponent*>(rhs);
	auto l_resourceBinder = addResourcesBinder();

	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Sampler;

	l_resourceBinder->m_Sampler.SamplerDesc.Filter = GetFilterMode(l_rhs->m_SamplerDesc.m_MinFilterMethod);
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

	D3D12_RESOURCE_STATES l_resourceState = l_rhs->m_GPUBufferAccessibility == GPUBufferAccessibility::ReadOnly ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	auto l_HResult = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(l_rhs->m_TotalSize),
		l_resourceState,
		nullptr,
		IID_PPV_ARGS(&l_rhs->m_ResourceHandle));

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX12RenderingServer: Can't create GPU buffer!");
		return false;
	}

	CD3DX12_RANGE m_ConstantBufferReadRange(0, 0);
	l_rhs->m_ResourceHandle->Map(0, &m_ConstantBufferReadRange, &l_rhs->m_MappedMemory);

	if (l_rhs->m_InitialData)
	{
		UploadGPUBufferDataComponent(l_rhs, l_rhs->m_InitialData);
	}

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	return true;
}

bool DX12RenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteSamplerDataComponent(SamplerDataComponent * rhs)
{
	return false;
}

bool DX12RenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return false;
}

bool DX12RenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferDataComponent*>(rhs);

	std::memcpy(l_rhs->m_MappedMemory, GPUBufferValue, l_rhs->m_TotalSize);

	return true;
}

bool DX12RenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[frameIndex]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	l_rhs->m_CurrentFrame = frameIndex;
	l_rhs->m_CommandAllocators[frameIndex]->Reset();
	l_commandList->m_CommandList->Reset(l_rhs->m_CommandAllocators[frameIndex], l_PSO->m_PSO);

	return true;
}

bool DX12RenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame]);
		l_commandList->m_CommandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
	else
	{
		for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_RenderTargets[i]);

			l_commandList->m_CommandList->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUHeap, m_samplerHeap };

	l_commandList->m_CommandList->SetDescriptorHeaps(2, l_heaps);

	l_commandList->m_CommandList->SetGraphicsRootSignature(l_rhs->m_RootSignature);
	l_commandList->m_CommandList->RSSetViewports(1, &l_PSO->m_Viewport);
	l_commandList->m_CommandList->RSSetScissorRects(1, &l_PSO->m_Scissor);

	l_commandList->m_CommandList->SetPipelineState(l_PSO->m_PSO);

	D3D12_CPU_DESCRIPTOR_HANDLE* l_DSVDescriptorCPUHandle = NULL;

	if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		l_DSVDescriptorCPUHandle = &l_rhs->m_DSVDescriptorCPUHandle;
	}

	if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		l_commandList->m_CommandList->OMSetRenderTargets(1, &l_rhs->m_RTVDescriptorCPUHandles[l_rhs->m_CurrentFrame], FALSE, l_DSVDescriptorCPUHandle);
	}
	else
	{
		l_commandList->m_CommandList->OMSetRenderTargets((unsigned int)l_rhs->m_RenderPassDesc.m_RenderTargetCount, &l_rhs->m_RTVDescriptorCPUHandles[0], FALSE, l_DSVDescriptorCPUHandle);
	}

	return true;
}

bool DX12RenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	const float l_clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		l_commandList->m_CommandList->ClearRenderTargetView(l_rhs->m_RTVDescriptorCPUHandles[l_rhs->m_CurrentFrame], l_clearColor, 0, nullptr);
	}
	else
	{
		for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			l_commandList->m_CommandList->ClearRenderTargetView(l_rhs->m_RTVDescriptorCPUHandles[i], l_clearColor, 0, nullptr);
		}
	}

	if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		l_commandList->m_CommandList->ClearDepthStencilView(l_rhs->m_DSVDescriptorCPUHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x00, 0, nullptr);
	}

	return true;
}

bool DX12RenderingServer::ActivateResourceBinder(RenderPassDataComponent * renderPass, ShaderType shaderType, IResourceBinder * binder, size_t bindingSlot)
{
	auto l_binder = reinterpret_cast<DX12ResourceBinder*>(binder);
	auto l_renderPass = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	switch (l_binder->m_ResourceBinderType)
	{
	case ResourceBinderType::Sampler:
		l_commandList->m_CommandList->SetGraphicsRootDescriptorTable((unsigned int)bindingSlot, l_binder->m_Sampler.GPUHandle);
		break;
	case ResourceBinderType::Image:
		l_commandList->m_CommandList->SetGraphicsRootDescriptorTable((unsigned int)bindingSlot, l_binder->m_SRV.GPUHandle);
		break;
	case ResourceBinderType::ROBuffer:
		break;
	case ResourceBinderType::ROBufferArray:
		break;
	case ResourceBinderType::RWBuffer:
		break;
	case ResourceBinderType::RWBufferArray:
		break;
	default:
		break;
	}

	return true;
}

bool DX12RenderingServer::BindGPUBufferDataComponent(RenderPassDataComponent * renderPass, GPUBufferDataComponent * GPUBuffer, ShaderType shaderType, GPUBufferAccessibility accessibility, size_t startOffset, size_t range)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_GPUBuffer = reinterpret_cast<DX12GPUBufferDataComponent*>(GPUBuffer);

	l_commandList->m_CommandList->SetGraphicsRootConstantBufferView((unsigned int)l_GPUBuffer->m_BindingPoint, l_GPUBuffer->m_ResourceHandle->GetGPUVirtualAddress() + startOffset * range);

	return true;
}

bool DX12RenderingServer::BindShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::BindMaterialDataComponent(RenderPassDataComponent * renderPass, ShaderType shaderType, MaterialDataComponent * rhs)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		auto l_material = reinterpret_cast<DX12MaterialDataComponent*>(rhs);

		if (l_material->m_SRVs.size() > 0)
		{
			l_commandList->m_CommandList->SetGraphicsRootDescriptorTable(0, l_material->m_SRVs[0].GPUHandle);
		}
	}
	else
	{
		auto l_material = reinterpret_cast<DX12MaterialDataComponent*>(g_pModuleManager->getRenderingFrontend()->getDefaultMaterialDataComponent());
		l_commandList->m_CommandList->SetGraphicsRootDescriptorTable(0, l_material->m_SRVs[0].GPUHandle);
	}

	return true;
}

bool DX12RenderingServer::DispatchDrawCall(RenderPassDataComponent* renderPass, MeshDataComponent* mesh)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassDataComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<DX12MeshDataComponent*>(mesh);

	l_commandList->m_CommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_CommandList->IASetVertexBuffers(0, 1, &l_mesh->m_VBV);
	l_commandList->m_CommandList->IASetIndexBuffer(&l_mesh->m_IBV);
	l_commandList->m_CommandList->DrawIndexedInstanced((unsigned int)l_mesh->m_indicesSize, 1, 0, 0, 0);

	return true;
}

bool DX12RenderingServer::DeactivateResourceBinder(RenderPassDataComponent * renderPass, ShaderType shaderType, IResourceBinder * binder, size_t bindingSlot)
{
	return true;
}

bool DX12RenderingServer::UnbindMaterialDataComponent(RenderPassDataComponent * renderPass, ShaderType shaderType, MaterialDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::CommandListEnd(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame]);
		l_commandList->m_CommandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
	else
	{
		for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_DX12TDC = reinterpret_cast<DX12TextureDataComponent*>(l_rhs->m_RenderTargets[i]);

			l_commandList->m_CommandList->ResourceBarrier(1,
				&CD3DX12_RESOURCE_BARRIER::Transition(l_DX12TDC->m_ResourceHandle, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		}
	}

	l_commandList->m_CommandList->Close();

	return true;
}

bool DX12RenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassDataComponent*>(rhs);
	auto l_commandQueue = reinterpret_cast<DX12CommandQueue*>(l_rhs->m_CommandQueue);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_fence = reinterpret_cast<DX12Fence*>(l_rhs->m_Fences[l_rhs->m_CurrentFrame]);

	ID3D12CommandList* l_commandLists[] = { l_commandList->m_CommandList };

	l_commandQueue->m_CommandQueue->ExecuteCommandLists(1, l_commandLists);

	// Schedule a Signal command in the queue.
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

RenderPassDataComponent * DX12RenderingServer::GetSwapChainRPDC()
{
	return m_SwapChainRPDC;
}

bool DX12RenderingServer::Present()
{
	// Present the frame.
	m_swapChain->Present(1, 0);

	// Schedule a Signal command in the queue.
	auto l_currentFence = reinterpret_cast<DX12Fence*>(m_SwapChainRPDC->m_Fences[m_SwapChainRPDC->m_CurrentFrame]);
	const UINT64 l_currentFenceValue = l_currentFence->m_FenceStatus;

	auto l_commandQueue = reinterpret_cast<DX12CommandQueue*>(m_SwapChainRPDC->m_CommandQueue);
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

bool DX12RenderingServer::CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool DX12RenderingServer::CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool DX12RenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	return true;
}

vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return vec4();
}

std::vector<vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	return std::vector<vec4>();
}

bool DX12RenderingServer::Resize()
{
	return true;
}

bool DX12RenderingServer::ReloadShader(RenderPassType renderPassType)
{
	return true;
}

bool DX12RenderingServer::BakeGIData()
{
	return true;
}

DX12SRV DX12RenderingServer::CreateSRV(TextureDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureDataComponent*>(rhs);

	DX12SRV l_result;

	unsigned int l_mipLevels = -1;
	if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
		|| l_rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		l_mipLevels = 1;
	}

	l_result.SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	l_result.SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	l_result.SRVDesc.Texture2D.MostDetailedMip = 0;
	l_result.SRVDesc.Texture2D.MipLevels = l_mipLevels;

	if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_result.SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	}
	else if (l_rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_result.SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	else
	{
		l_result.SRVDesc.Format = l_rhs->m_DX12TextureDataDesc.Format;
	}

	l_result.CPUHandle = m_currentCSUCPUHandle;
	l_result.GPUHandle = m_currentCSUGPUHandle;

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_currentCSUCPUHandle.ptr += l_CSUDescSize;
	m_currentCSUGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateShaderResourceView(l_rhs->m_ResourceHandle, &l_result.SRVDesc, l_result.CPUHandle);

	return l_result;
}

DX12CBV DX12RenderingServer::CreateCBV(GPUBufferDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12GPUBufferDataComponent*>(rhs);

	DX12CBV l_result;

	l_result.CBVDesc.BufferLocation = l_rhs->m_ResourceHandle->GetGPUVirtualAddress();
	l_result.CBVDesc.SizeInBytes = (unsigned int)l_rhs->m_ElementSize;

	l_result.CPUHandle = m_currentCSUCPUHandle;
	l_result.GPUHandle = m_currentCSUGPUHandle;

	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_currentCSUCPUHandle.ptr += l_CSUDescSize;
	m_currentCSUGPUHandle.ptr += l_CSUDescSize;

	m_device->CreateConstantBufferView(&l_result.CBVDesc, l_result.CPUHandle);

	return l_result;
}
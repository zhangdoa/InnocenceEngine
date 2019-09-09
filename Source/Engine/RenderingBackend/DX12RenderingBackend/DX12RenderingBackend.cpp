#include "DX12RenderingBackend.h"

#include "DX12RenderingBackendUtilities.h"

#include "DX12OpaquePass.h"
#include "DX12LightPass.h"
#include "DX12FinalBlendPass.h"

#include "../../Component/DX12RenderingBackendComponent.h"
#include "../../Component/WinWindowSystemComponent.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace DX12RenderingBackendNS
{
	IDXGIAdapter1* getHardwareAdapter(IDXGIFactory2* pFactory)
	{
		IDXGIAdapter1* l_adapter;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &l_adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			l_adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(l_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		return l_adapter;
	}

	bool createDebugCallback();
	bool createPhysicalDevices();
	bool createGlobalCommandQueue();
	bool createGlobalCommandAllocator();

	bool createGlobalCSUHeap();
	bool createGlobalSamplerHeap();
	bool createSwapChain();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	EntityID m_EntityID;

	static DX12RenderingBackendComponent* g_DXRenderingBackendComponent;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<DX12MeshDataComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<DX12MaterialDataComponent*> m_uninitializedMaterials;

	DX12TextureDataComponent* m_iconTemplate_OBJ;
	DX12TextureDataComponent* m_iconTemplate_PNG;
	DX12TextureDataComponent* m_iconTemplate_SHADER;
	DX12TextureDataComponent* m_iconTemplate_UNKNOWN;

	DX12TextureDataComponent* m_iconTemplate_DirectionalLight;
	DX12TextureDataComponent* m_iconTemplate_PointLight;
	DX12TextureDataComponent* m_iconTemplate_SphereLight;

	DX12MeshDataComponent* m_unitLineMDC;
	DX12MeshDataComponent* m_unitQuadMDC;
	DX12MeshDataComponent* m_unitCubeMDC;
	DX12MeshDataComponent* m_unitSphereMDC;
	DX12MeshDataComponent* m_terrainMDC;

	DX12TextureDataComponent* m_basicNormalTexture;
	DX12TextureDataComponent* m_basicAlbedoTexture;
	DX12TextureDataComponent* m_basicMetallicTexture;
	DX12TextureDataComponent* m_basicRoughnessTexture;
	DX12TextureDataComponent* m_basicAOTexture;

	DX12MaterialDataComponent* m_basicMaterial;
}

bool DX12RenderingBackendNS::createDebugCallback()
{
	ID3D12Debug* l_debugInterface;

	auto l_result = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't get DirectX 12 debug interface!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	l_result = l_debugInterface->QueryInterface(IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_debugInterface));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't query DirectX 12 debug interface!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_DXRenderingBackendComponent->m_debugInterface->EnableDebugLayer();
	g_DXRenderingBackendComponent->m_debugInterface->SetEnableGPUBasedValidation(true);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: Debug layer and GPU based validation has been enabled.");

	return true;
}

bool DX12RenderingBackendNS::createPhysicalDevices()
{
	HRESULT l_result;

	uint32_t numModes;
	uint64_t stringLength;

	// Create a DirectX graphics interface factory.
	l_result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_factory));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create DXGI factory!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: DXGI factory has been created.");

	// Use the factory to create an adapter for the primary graphics interface (video card).
	auto l_adapter1 = getHardwareAdapter(g_DXRenderingBackendComponent->m_factory);

	if (l_adapter1 == nullptr)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create a suitable video card adapter!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_DXRenderingBackendComponent->m_adapter = reinterpret_cast<IDXGIAdapter4*>(l_adapter1);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: Video card adapter has been created.");

	// Enumerate the primary adapter output (monitor).
	IDXGIOutput* l_adapterOutput;

	l_result = g_DXRenderingBackendComponent->m_adapter->EnumOutputs(0, &l_adapterOutput);
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't create monitor adapter!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	l_result = l_adapterOutput->QueryInterface(IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_adapterOutput));

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	l_result = g_DXRenderingBackendComponent->m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	std::vector<DXGI_MODE_DESC1> displayModeList(numModes);

	// Now fill the display mode list structures.
	l_result = g_DXRenderingBackendComponent->m_adapterOutput->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, &displayModeList[0]);
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't fill the display mode list structures!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	for (uint32_t i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == l_screenResolution.x
			&&
			displayModeList[i].Height == l_screenResolution.y
			)
		{
			g_DXRenderingBackendComponent->m_refreshRate.x = displayModeList[i].RefreshRate.Numerator;
			g_DXRenderingBackendComponent->m_refreshRate.y = displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Get the adapter (video card) description.
	l_result = g_DXRenderingBackendComponent->m_adapter->GetDesc(&g_DXRenderingBackendComponent->m_adapterDesc);
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't get the video card adapter description!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	g_DXRenderingBackendComponent->m_videoCardMemory = (int32_t)(g_DXRenderingBackendComponent->m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	if (wcstombs_s(&stringLength, g_DXRenderingBackendComponent->m_videoCardDescription, 128, g_DXRenderingBackendComponent->m_adapterDesc.Description, 128) != 0)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't convert the name of the video card to a character array!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Release the display mode list.
	// displayModeList.clear();

	// Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
	// Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
	auto featureLevel = D3D_FEATURE_LEVEL_12_1;

	// Create the Direct3D 12 device.
	l_result = D3D12CreateDevice(g_DXRenderingBackendComponent->m_adapter, featureLevel, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_device));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create a DirectX 12.1 device. The default video card does not support DirectX 12.1!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: D3D device has been created.");

	// Set debug report severity
	ID3D12InfoQueue* l_pInfoQueue;
	l_result = g_DXRenderingBackendComponent->m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue));

	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	//l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

	return true;
}

bool DX12RenderingBackendNS::createGlobalCommandQueue()
{
	// Set up the description of the command queue.
	g_DXRenderingBackendComponent->m_globalCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	g_DXRenderingBackendComponent->m_globalCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	g_DXRenderingBackendComponent->m_globalCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	g_DXRenderingBackendComponent->m_globalCommandQueueDesc.NodeMask = 0;

	// Create the command queue.
	auto l_result = g_DXRenderingBackendComponent->m_device->CreateCommandQueue(&g_DXRenderingBackendComponent->m_globalCommandQueueDesc, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_globalCommandQueue));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create global CommandQueue!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_DXRenderingBackendComponent->m_globalCommandQueue->SetName(L"GlobalCommandQueue");

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: Global CommandQueue has been created.");

	return true;
}

bool DX12RenderingBackendNS::createGlobalCommandAllocator()
{
	auto l_result = g_DXRenderingBackendComponent->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_globalCommandAllocator));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create global CommandAllocator!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_DXRenderingBackendComponent->m_globalCommandAllocator->SetName(L"GlobalCommandAllocator");

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: Global CommandAllocator has been created.");

	return true;
}

bool DX12RenderingBackendNS::createGlobalCSUHeap()
{
	g_DXRenderingBackendComponent->m_CSUHeapDesc.NumDescriptors = 65536;
	g_DXRenderingBackendComponent->m_CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	g_DXRenderingBackendComponent->m_CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = DX12RenderingBackendComponent::get().m_device->CreateDescriptorHeap(&g_DXRenderingBackendComponent->m_CSUHeapDesc, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_CSUHeap));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create DescriptorHeap for CBV/SRV/UAV!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_DXRenderingBackendComponent->m_CSUHeap->SetName(L"GlobalCSUHeap");

	g_DXRenderingBackendComponent->m_initialCSUCPUHandle = g_DXRenderingBackendComponent->m_CSUHeap->GetCPUDescriptorHandleForHeapStart();
	g_DXRenderingBackendComponent->m_initialCSUGPUHandle = g_DXRenderingBackendComponent->m_CSUHeap->GetGPUDescriptorHandleForHeapStart();

	g_DXRenderingBackendComponent->m_currentCSUCPUHandle = g_DXRenderingBackendComponent->m_initialCSUCPUHandle;
	g_DXRenderingBackendComponent->m_currentCSUGPUHandle = g_DXRenderingBackendComponent->m_initialCSUGPUHandle;

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: DescriptorHeap for CBV/SRV/UAV has been created.");

	return true;
}

bool DX12RenderingBackendNS::createGlobalSamplerHeap()
{
	g_DXRenderingBackendComponent->m_samplerHeapDesc.NumDescriptors = 128;
	g_DXRenderingBackendComponent->m_samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	g_DXRenderingBackendComponent->m_samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = DX12RenderingBackendComponent::get().m_device->CreateDescriptorHeap(&g_DXRenderingBackendComponent->m_samplerHeapDesc, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_samplerHeap));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create DescriptorHeap for Sampler!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_DXRenderingBackendComponent->m_samplerHeap->SetName(L"GlobalSamplerHeap");

	g_DXRenderingBackendComponent->m_initialSamplerCPUHandle = g_DXRenderingBackendComponent->m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
	g_DXRenderingBackendComponent->m_initialSamplerGPUHandle = g_DXRenderingBackendComponent->m_samplerHeap->GetGPUDescriptorHandleForHeapStart();

	g_DXRenderingBackendComponent->m_currentSamplerCPUHandle = g_DXRenderingBackendComponent->m_initialSamplerCPUHandle;
	g_DXRenderingBackendComponent->m_currentSamplerGPUHandle = g_DXRenderingBackendComponent->m_initialSamplerGPUHandle;

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: DescriptorHeap for Sampler has been created.");

	return true;
}

bool DX12RenderingBackendNS::createSwapChain()
{
	auto l_imageCount = 2;
	// create swap chain
	// Set the swap chain to use double buffering.
	g_DXRenderingBackendComponent->m_swapChainDesc.BufferCount = l_imageCount;

	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	// Set the width and height of the back buffer.
	g_DXRenderingBackendComponent->m_swapChainDesc.Width = (UINT)l_screenResolution.x;
	g_DXRenderingBackendComponent->m_swapChainDesc.Height = (UINT)l_screenResolution.y;

	// Set regular 32-bit surface for the back buffer.
	g_DXRenderingBackendComponent->m_swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the usage of the back buffer.
	g_DXRenderingBackendComponent->m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Turn multisampling off.
	g_DXRenderingBackendComponent->m_swapChainDesc.SampleDesc.Count = 1;
	g_DXRenderingBackendComponent->m_swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	// @TODO: finish this feature

	// Discard the back buffer contents after presenting.
	g_DXRenderingBackendComponent->m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Don't set the advanced flags.
	g_DXRenderingBackendComponent->m_swapChainDesc.Flags = 0;

	// Finally create the swap chain using the swap chain description.
	IDXGISwapChain1* l_swapChain1;
	auto l_hResult = g_DXRenderingBackendComponent->m_factory->CreateSwapChainForHwnd(
		DX12FinalBlendPass::getDX12RPC()->m_commandQueue,
		WinWindowSystemComponent::get().m_hwnd,
		&g_DXRenderingBackendComponent->m_swapChainDesc,
		nullptr,
		nullptr,
		&l_swapChain1);

	l_hResult = l_swapChain1->QueryInterface(IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_swapChain));

	if (FAILED(l_hResult))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create swap chain!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: Swap chain has been created.");

	DX12RenderingBackendComponent::get().m_swapChainImages.resize(l_imageCount);

	// use device created swap chain textures
	for (size_t i = 0; i < l_imageCount; i++)
	{
		auto l_result = DX12RenderingBackendComponent::get().m_swapChain->GetBuffer((uint32_t)i, IID_PPV_ARGS(&DX12RenderingBackendComponent::get().m_swapChainImages[i]));
		if (FAILED(l_result))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12FinalBlendPass: Can't get pointer of swap chain render target " + std::to_string(i) + "!");
			return false;
		}
	}

	return true;
}

bool DX12RenderingBackendNS::setup()
{
	m_EntityID = InnoMath::createEntityID();

	g_DXRenderingBackendComponent = &DX12RenderingBackendComponent::get();

	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	// general render pass desc
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTNumber = 1;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.width = l_screenResolution.x;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.height = l_screenResolution.y;
	g_DXRenderingBackendComponent->m_deferredRenderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;

	bool l_result = true;
	l_result &= initializeComponentPool();

	l_result &= createDebugCallback();
	l_result &= createPhysicalDevices();

	m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend setup finished.");

	return l_result;
}

bool DX12RenderingBackendNS::initialize()
{
	if (DX12RenderingBackendNS::m_ObjectStatus == ObjectStatus::Created)
	{
		auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

		m_MeshDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(DX12MeshDataComponent), l_renderingCapability.maxMeshes);
		m_MaterialDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(DX12MaterialDataComponent), l_renderingCapability.maxMaterials);
		m_TextureDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(DX12TextureDataComponent), l_renderingCapability.maxTextures);

		bool l_result = true;

		l_result &= createGlobalCommandQueue();
		l_result &= createGlobalCommandAllocator();

		l_result &= createGlobalCSUHeap();
		l_result &= createGlobalSamplerHeap();

		loadDefaultAssets();

		generateGPUBuffers();

		DX12FinalBlendPass::setup();
		l_result &= createSwapChain();
		DX12FinalBlendPass::initialize();

		DX12OpaquePass::initialize();
		DX12LightPass::initialize();

		DX12RenderingBackendNS::m_ObjectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend has been initialized.");
		return l_result;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Object is not created!");
		return false;
	}
}

bool DX12RenderingBackendNS::update()
{
	while (DX12RenderingBackendNS::m_uninitializedMeshes.size() > 0)
	{
		DX12MeshDataComponent* l_MDC;
		DX12RenderingBackendNS::m_uninitializedMeshes.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeDX12MeshDataComponent(l_MDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't initialize DX12MeshDataComponent for " + std::string(l_MDC->m_ParentEntity->m_EntityName.c_str()) + "!");
			}
		}
	}
	while (DX12RenderingBackendNS::m_uninitializedMaterials.size() > 0)
	{
		DX12MaterialDataComponent* l_MDC;
		DX12RenderingBackendNS::m_uninitializedMaterials.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeDX12MaterialDataComponent(l_MDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't initialize DX12TextureDataComponent for " + std::string(l_MDC->m_ParentEntity->m_EntityName.c_str()) + "!");
			}
		}
	}

	updateConstantBuffer(DX12RenderingBackendComponent::get().m_cameraConstantBuffer, g_pModuleManager->getRenderingFrontend()->getCameraGPUData());
	updateConstantBuffer(DX12RenderingBackendComponent::get().m_sunConstantBuffer, g_pModuleManager->getRenderingFrontend()->getSunGPUData());
	updateConstantBuffer(DX12RenderingBackendComponent::get().m_pointLightConstantBuffer, g_pModuleManager->getRenderingFrontend()->getPointLightGPUData());
	updateConstantBuffer(DX12RenderingBackendComponent::get().m_sphereLightConstantBuffer, g_pModuleManager->getRenderingFrontend()->getSphereLightGPUData());
	updateConstantBuffer(DX12RenderingBackendComponent::get().m_skyConstantBuffer, g_pModuleManager->getRenderingFrontend()->getSkyGPUData());
	updateConstantBuffer(DX12RenderingBackendComponent::get().m_meshConstantBuffer, g_pModuleManager->getRenderingFrontend()->getOpaquePassMeshGPUData());
	updateConstantBuffer(DX12RenderingBackendComponent::get().m_materialConstantBuffer, g_pModuleManager->getRenderingFrontend()->getOpaquePassMaterialGPUData());

	DX12OpaquePass::update();
	DX12LightPass::update();
	DX12FinalBlendPass::update();

	return true;
}

bool DX12RenderingBackendNS::render()
{
	DX12OpaquePass::render();
	DX12LightPass::render();
	DX12FinalBlendPass::render();

	return true;
}

bool DX12RenderingBackendNS::terminate()
{
	DX12FinalBlendPass::terminate();
	DX12LightPass::terminate();
	DX12OpaquePass::terminate();

	destroyAllGraphicPrimitiveComponents();

	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (g_DXRenderingBackendComponent->m_swapChain)
	{
		g_DXRenderingBackendComponent->m_swapChain->SetFullscreenState(false, NULL);
	}

	if (g_DXRenderingBackendComponent->m_CSUHeap)
	{
		g_DXRenderingBackendComponent->m_CSUHeap->Release();
		g_DXRenderingBackendComponent->m_CSUHeap = 0;
	}

	if (g_DXRenderingBackendComponent->m_samplerHeap)
	{
		g_DXRenderingBackendComponent->m_samplerHeap->Release();
		g_DXRenderingBackendComponent->m_samplerHeap = 0;
	}

	g_DXRenderingBackendComponent->m_cameraConstantBuffer.m_constantBuffer->Release();
	g_DXRenderingBackendComponent->m_meshConstantBuffer.m_constantBuffer->Release();
	g_DXRenderingBackendComponent->m_materialConstantBuffer.m_constantBuffer->Release();
	g_DXRenderingBackendComponent->m_sunConstantBuffer.m_constantBuffer->Release();
	g_DXRenderingBackendComponent->m_pointLightConstantBuffer.m_constantBuffer->Release();
	g_DXRenderingBackendComponent->m_sphereLightConstantBuffer.m_constantBuffer->Release();
	g_DXRenderingBackendComponent->m_skyConstantBuffer.m_constantBuffer->Release();

	if (g_DXRenderingBackendComponent->m_globalCommandAllocator)
	{
		g_DXRenderingBackendComponent->m_globalCommandAllocator->Release();
		g_DXRenderingBackendComponent->m_globalCommandAllocator = 0;
	}

	if (g_DXRenderingBackendComponent->m_swapChain)
	{
		g_DXRenderingBackendComponent->m_swapChain->Release();
		g_DXRenderingBackendComponent->m_swapChain = 0;
	}

	if (g_DXRenderingBackendComponent->m_globalCommandQueue)
	{
		g_DXRenderingBackendComponent->m_globalCommandQueue->Release();
		g_DXRenderingBackendComponent->m_globalCommandQueue = 0;
	}

	g_DXRenderingBackendComponent->m_debugInterface->Release();

#if defined(_DEBUG)
	ID3D12DebugDevice1* l_debugDevice;
	auto l_result = g_DXRenderingBackendComponent->m_device->QueryInterface(IID_PPV_ARGS(&l_debugDevice));
	l_debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
#endif

	if (g_DXRenderingBackendComponent->m_device)
	{
		g_DXRenderingBackendComponent->m_device->Release();
		g_DXRenderingBackendComponent->m_device = 0;
	}

	if (g_DXRenderingBackendComponent->m_adapter)
	{
		g_DXRenderingBackendComponent->m_adapter->Release();
		g_DXRenderingBackendComponent->m_adapter = 0;
	}

	if (g_DXRenderingBackendComponent->m_factory)
	{
		g_DXRenderingBackendComponent->m_factory->Release();
		g_DXRenderingBackendComponent->m_factory = 0;
	}

	m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend has been terminated.");
	return true;
}

void DX12RenderingBackendNS::loadDefaultAssets()
{
	auto l_basicNormalTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_basicAlbedoTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	auto l_basicMetallicTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	auto l_basicRoughnessTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	auto l_basicAOTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	auto l_iconTemplate_OBJ = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PNG = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SHADER = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_UNKNOWN = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	auto l_iconTemplate_DirectionalLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PointLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SphereLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_basicNormalTexture = reinterpret_cast<DX12TextureDataComponent*>(l_basicNormalTDC);
	m_basicAlbedoTexture = reinterpret_cast<DX12TextureDataComponent*>(l_basicAlbedoTDC);
	m_basicMetallicTexture = reinterpret_cast<DX12TextureDataComponent*>(l_basicMetallicTDC);
	m_basicRoughnessTexture = reinterpret_cast<DX12TextureDataComponent*>(l_basicRoughnessTDC);
	m_basicAOTexture = reinterpret_cast<DX12TextureDataComponent*>(l_basicAOTDC);

	m_iconTemplate_OBJ = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_OBJ);
	m_iconTemplate_PNG = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_PNG);
	m_iconTemplate_SHADER = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_SHADER);
	m_iconTemplate_UNKNOWN = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_UNKNOWN);

	m_iconTemplate_DirectionalLight = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_DirectionalLight);
	m_iconTemplate_PointLight = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_PointLight);
	m_iconTemplate_SphereLight = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_SphereLight);

	m_unitLineMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMDC);
	m_unitQuadMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitQuadMDC->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMDC->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMDC);

	m_unitCubeMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

	m_basicMaterial = addDX12MaterialDataComponent();
	m_basicMaterial->m_normalTexture = m_basicNormalTexture;
	m_basicMaterial->m_albedoTexture = m_basicAlbedoTexture;
	m_basicMaterial->m_metallicTexture = m_basicMetallicTexture;
	m_basicMaterial->m_roughnessTexture = m_basicRoughnessTexture;
	m_basicMaterial->m_aoTexture = m_basicAOTexture;

	initializeDX12MeshDataComponent(m_unitLineMDC);
	initializeDX12MeshDataComponent(m_unitQuadMDC);
	initializeDX12MeshDataComponent(m_unitCubeMDC);
	initializeDX12MeshDataComponent(m_unitSphereMDC);
	initializeDX12MeshDataComponent(m_terrainMDC);

	initializeDX12TextureDataComponent(m_basicNormalTexture);
	initializeDX12TextureDataComponent(m_basicAlbedoTexture);
	initializeDX12TextureDataComponent(m_basicMetallicTexture);
	initializeDX12TextureDataComponent(m_basicRoughnessTexture);
	initializeDX12TextureDataComponent(m_basicAOTexture);

	initializeDX12TextureDataComponent(m_iconTemplate_OBJ);
	initializeDX12TextureDataComponent(m_iconTemplate_PNG);
	initializeDX12TextureDataComponent(m_iconTemplate_SHADER);
	initializeDX12TextureDataComponent(m_iconTemplate_UNKNOWN);

	initializeDX12TextureDataComponent(m_iconTemplate_DirectionalLight);
	initializeDX12TextureDataComponent(m_iconTemplate_PointLight);
	initializeDX12TextureDataComponent(m_iconTemplate_SphereLight);

	initializeDX12MaterialDataComponent(m_basicMaterial);
}

bool DX12RenderingBackendNS::generateGPUBuffers()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	g_DXRenderingBackendComponent->m_cameraConstantBuffer = createConstantBuffer(sizeof(CameraGPUData), 1, L"cameraConstantBuffer");
	g_DXRenderingBackendComponent->m_meshConstantBuffer = createConstantBuffer(sizeof(MeshGPUData), l_renderingCapability.maxMeshes, L"meshConstantBuffer");
	g_DXRenderingBackendComponent->m_materialConstantBuffer = createConstantBuffer(sizeof(MaterialGPUData), l_renderingCapability.maxMaterials, L"materialConstantBuffer");
	g_DXRenderingBackendComponent->m_sunConstantBuffer = createConstantBuffer(sizeof(SunGPUData), 1, L"sunConstantBuffer");
	g_DXRenderingBackendComponent->m_pointLightConstantBuffer = createConstantBuffer(sizeof(PointLightGPUData), l_renderingCapability.maxPointLights, L"pointLightConstantBuffer");
	g_DXRenderingBackendComponent->m_sphereLightConstantBuffer = createConstantBuffer(sizeof(SphereLightGPUData), l_renderingCapability.maxSphereLights, L"sphereLightConstantBuffer");
	g_DXRenderingBackendComponent->m_skyConstantBuffer = createConstantBuffer(sizeof(SkyGPUData), 1, L"skyConstantBuffer");

	return true;
}

DX12MeshDataComponent* DX12RenderingBackendNS::addDX12MeshDataComponent()
{
	static std::atomic<uint32_t> meshCount = 0;
	meshCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(DX12MeshDataComponent));
	auto l_MDC = new(l_rawPtr)DX12MeshDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Mesh_" + std::to_string(meshCount) + "/").c_str());
	l_MDC->m_ParentEntity = l_parentEntity;
	return l_MDC;
}

DX12MaterialDataComponent* DX12RenderingBackendNS::addDX12MaterialDataComponent()
{
	static std::atomic<uint32_t> materialCount = 0;
	materialCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(DX12MaterialDataComponent));
	auto l_MDC = new(l_rawPtr)DX12MaterialDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Material_" + std::to_string(materialCount) + "/").c_str());
	l_MDC->m_ParentEntity = l_parentEntity;
	return l_MDC;
}

DX12TextureDataComponent* DX12RenderingBackendNS::addDX12TextureDataComponent()
{
	static std::atomic<uint32_t> textureCount = 0;
	textureCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(DX12TextureDataComponent));
	auto l_TDC = new(l_rawPtr)DX12TextureDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Texture_" + std::to_string(textureCount) + "/").c_str());
	l_TDC->m_ParentEntity = l_parentEntity;
	return l_TDC;
}

DX12MeshDataComponent* DX12RenderingBackendNS::getDX12MeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return DX12RenderingBackendNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return DX12RenderingBackendNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return DX12RenderingBackendNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return DX12RenderingBackendNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return DX12RenderingBackendNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: wrong MeshShapeType passed to DX12RenderingBackend::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

DX12TextureDataComponent * DX12RenderingBackendNS::getDX12TextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return DX12RenderingBackendNS::m_basicNormalTexture; break;
	case TextureUsageType::ALBEDO:
		return DX12RenderingBackendNS::m_basicAlbedoTexture; break;
	case TextureUsageType::METALLIC:
		return DX12RenderingBackendNS::m_basicMetallicTexture; break;
	case TextureUsageType::ROUGHNESS:
		return DX12RenderingBackendNS::m_basicRoughnessTexture; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return DX12RenderingBackendNS::m_basicAOTexture; break;
	case TextureUsageType::COLOR_ATTACHMENT:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

DX12TextureDataComponent * DX12RenderingBackendNS::getDX12TextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return DX12RenderingBackendNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return DX12RenderingBackendNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return DX12RenderingBackendNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return DX12RenderingBackendNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

DX12TextureDataComponent * DX12RenderingBackendNS::getDX12TextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return DX12RenderingBackendNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return DX12RenderingBackendNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return DX12RenderingBackendNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

DX12MaterialDataComponent * DX12RenderingBackendNS::getDefaultMaterialDataComponent()
{
	return DX12RenderingBackendNS::m_basicMaterial;
}

bool DX12RenderingBackendNS::resize()
{
	return true;
}

bool DX12RenderingBackend::setup()
{
	return DX12RenderingBackendNS::setup();
}

bool DX12RenderingBackend::initialize()
{
	return DX12RenderingBackendNS::initialize();
}

bool DX12RenderingBackend::update()
{
	return DX12RenderingBackendNS::update();
}

bool DX12RenderingBackend::render()
{
	return DX12RenderingBackendNS::render();
}

bool DX12RenderingBackend::present()
{
	return true;
}

bool DX12RenderingBackend::terminate()
{
	return DX12RenderingBackendNS::terminate();
}

ObjectStatus DX12RenderingBackend::getStatus()
{
	return DX12RenderingBackendNS::m_ObjectStatus;
}

MeshDataComponent * DX12RenderingBackend::addMeshDataComponent()
{
	return DX12RenderingBackendNS::addDX12MeshDataComponent();
}

MaterialDataComponent * DX12RenderingBackend::addMaterialDataComponent()
{
	return DX12RenderingBackendNS::addDX12MaterialDataComponent();
}

TextureDataComponent * DX12RenderingBackend::addTextureDataComponent()
{
	return DX12RenderingBackendNS::addDX12TextureDataComponent();
}

MeshDataComponent * DX12RenderingBackend::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return DX12RenderingBackendNS::getDX12MeshDataComponent(MeshShapeType);
}

TextureDataComponent * DX12RenderingBackend::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return DX12RenderingBackendNS::getDX12TextureDataComponent(TextureUsageType);
}

TextureDataComponent * DX12RenderingBackend::getTextureDataComponent(FileExplorerIconType iconType)
{
	return DX12RenderingBackendNS::getDX12TextureDataComponent(iconType);
}

TextureDataComponent * DX12RenderingBackend::getTextureDataComponent(WorldEditorIconType iconType)
{
	return DX12RenderingBackendNS::getDX12TextureDataComponent(iconType);
}

void DX12RenderingBackend::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	DX12RenderingBackendNS::m_uninitializedMeshes.push(reinterpret_cast<DX12MeshDataComponent*>(rhs));
}

void DX12RenderingBackend::registerUninitializedMaterialDataComponent(MaterialDataComponent * rhs)
{
	DX12RenderingBackendNS::m_uninitializedMaterials.push(reinterpret_cast<DX12MaterialDataComponent*>(rhs));
}

bool DX12RenderingBackend::resize()
{
	return DX12RenderingBackendNS::resize();
}

bool DX12RenderingBackend::reloadShader(RenderPassType renderPassType)
{
	switch (renderPassType)
	{
	case RenderPassType::Shadow:
		break;
	case RenderPassType::Opaque:
		break;
	case RenderPassType::Light:
		break;
	case RenderPassType::Transparent:
		break;
	case RenderPassType::Terrain:
		break;
	case RenderPassType::PostProcessing:
		break;
	default: break;
	}

	return true;
}

bool DX12RenderingBackend::bakeGI()
{
	return true;
}
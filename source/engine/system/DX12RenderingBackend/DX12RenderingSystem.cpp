#include "DX12RenderingSystem.h"

#include "../../component/DX12RenderingSystemComponent.h"
#include "../../component/WinWindowSystemComponent.h"

#include "DX12RenderingSystemUtilities.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DX12RenderingSystemNS
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

	bool setup(IRenderingFrontendSystem* renderingFrontend);
	bool update();
	bool terminate();

	bool initializeDefaultAssets();

	void prepareRenderingData();

	static DX12RenderingSystemComponent* g_DXRenderingSystemComponent;

	bool createPhysicalDevices();
	bool createDebugCallback();
	bool createSwapChain();
	bool createBackBuffer();
	bool createRenderPass();
	bool createCommandList();
	bool createSyncPrimitives();
	bool createRasterizer();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	EntityID m_entityID;

	IRenderingFrontendSystem* m_renderingFrontendSystem;

	CameraDataPack m_cameraDataPack;
	SunDataPack m_sunDataPack;
	std::vector<MeshDataPack> m_meshDataPack;
}

bool DX12RenderingSystemNS::createDebugCallback()
{
	ID3D12Debug* l_debugInterface;

	auto l_result = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't get DirectX 12 debug interface!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	l_result = l_debugInterface->QueryInterface(IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_debugInterface));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't query DirectX 12 debug interface!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_debugInterface->EnableDebugLayer();
	g_DXRenderingSystemComponent->m_debugInterface->SetEnableGPUBasedValidation(true);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Debug layer and GPU based validation has been enabled.");

	return true;
}

bool DX12RenderingSystemNS::createPhysicalDevices()
{
	HRESULT result;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_factory));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create DXGI factory!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: DXGI factory has been created.");

	// Use the factory to create an adapter for the primary graphics interface (video card).
	auto l_adapter1 = getHardwareAdapter(g_DXRenderingSystemComponent->m_factory);

	if (l_adapter1 == nullptr)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create a suitable video card adapter!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_adapter = reinterpret_cast<IDXGIAdapter4*>(l_adapter1);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Video card adapter has been created.");

	// Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
	// Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
	auto featureLevel = D3D_FEATURE_LEVEL_12_1;

	// Create the Direct3D 12 device.
	result = D3D12CreateDevice(g_DXRenderingSystemComponent->m_adapter, featureLevel, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_device));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create a DirectX 12.1 device. The default video card does not support DirectX 12.1!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: D3D device has been created.");

	// Set debug report severity
	auto l_pInfoQueue = reinterpret_cast<ID3D12InfoQueue*>(g_DXRenderingSystemComponent->m_device);

	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	//l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	//l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

	// Initialize the description of the command queue.
	ZeroMemory(&g_DXRenderingSystemComponent->m_commandQueueDesc, sizeof(g_DXRenderingSystemComponent->m_commandQueueDesc));

	// Set up the description of the command queue.
	g_DXRenderingSystemComponent->m_commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	g_DXRenderingSystemComponent->m_commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	g_DXRenderingSystemComponent->m_commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	g_DXRenderingSystemComponent->m_commandQueueDesc.NodeMask = 0;

	// Create the command queue.
	result = g_DXRenderingSystemComponent->m_device->CreateCommandQueue(&g_DXRenderingSystemComponent->m_commandQueueDesc, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_commandQueue));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create command queue!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Command queue has been created.");

	// Release the adapter.
	g_DXRenderingSystemComponent->m_adapter->Release();
	g_DXRenderingSystemComponent->m_adapter = 0;

	return true;
}

bool DX12RenderingSystemNS::createSwapChain()
{
	HRESULT result;

	// Initialize the swap chain description.
	ZeroMemory(&g_DXRenderingSystemComponent->m_swapChainDesc, sizeof(g_DXRenderingSystemComponent->m_swapChainDesc));

	// Set the swap chain to use double buffering.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferCount = 2;

	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	// Set the width and height of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.Width = (UINT)l_screenResolution.x;
	g_DXRenderingSystemComponent->m_swapChainDesc.Height = (UINT)l_screenResolution.y;

	// Set regular 32-bit surface for the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the usage of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Turn multisampling off.
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Count = 1;
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	// @TODO: finish this feature

	// Discard the back buffer contents after presenting.
	g_DXRenderingSystemComponent->m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Don't set the advanced flags.
	g_DXRenderingSystemComponent->m_swapChainDesc.Flags = 0;

	// Finally create the swap chain using the swap chain description.
	IDXGISwapChain1* l_swapChain1;
	result = g_DXRenderingSystemComponent->m_factory->CreateSwapChainForHwnd(
		g_DXRenderingSystemComponent->m_commandQueue,
		WinWindowSystemComponent::get().m_hwnd,
		&g_DXRenderingSystemComponent->m_swapChainDesc,
		nullptr,
		nullptr,
		&l_swapChain1);

	g_DXRenderingSystemComponent->m_swapChain = reinterpret_cast<IDXGISwapChain4*>(l_swapChain1);

	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create swap chain!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Swap chain has been created.");

	// Release the factory.
	g_DXRenderingSystemComponent->m_factory->Release();
	g_DXRenderingSystemComponent->m_factory = 0;

	return true;
}

bool DX12RenderingSystemNS::createBackBuffer()
{
	HRESULT result;

	unsigned int renderTargetViewDescriptorSize;

	// Initialize the render target view heap description for the two back buffers.
	ZeroMemory(&g_DXRenderingSystemComponent->m_renderTargetViewHeapDesc, sizeof(g_DXRenderingSystemComponent->m_renderTargetViewHeapDesc));

	g_DXRenderingSystemComponent->m_renderTargetViewHeapDesc.NumDescriptors = 2;
	g_DXRenderingSystemComponent->m_renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	g_DXRenderingSystemComponent->m_renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Create the render target view heap for the back buffers.
	result = g_DXRenderingSystemComponent->m_device->CreateDescriptorHeap(&g_DXRenderingSystemComponent->m_renderTargetViewHeapDesc, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_renderTargetViewHeap));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create render target view desc heap!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Render target view desc heap has been created.");

	// Get a handle to the starting memory location in the render target view heap to identify where the render target views will be located for the two back buffers.
	g_DXRenderingSystemComponent->m_renderTargetViewHandle = g_DXRenderingSystemComponent->m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// Get the size of the memory location for the render target view descriptors.
	renderTargetViewDescriptorSize = g_DXRenderingSystemComponent->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a pointer to the first back buffer from the swap chain.
	result = g_DXRenderingSystemComponent->m_swapChain->GetBuffer(0, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_backBufferRenderTarget[0]));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't get pointer of first back buffer!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create a render target view for the first back buffer.
	g_DXRenderingSystemComponent->m_device->CreateRenderTargetView(g_DXRenderingSystemComponent->m_backBufferRenderTarget[0], NULL, g_DXRenderingSystemComponent->m_renderTargetViewHandle);

	// Increment the view handle to the next descriptor location in the render target view heap.
	g_DXRenderingSystemComponent->m_renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	// Get a pointer to the second back buffer from the swap chain.
	result = g_DXRenderingSystemComponent->m_swapChain->GetBuffer(1, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_backBufferRenderTarget[1]));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't get pointer of second back buffer!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create a render target view for the second back buffer.
	g_DXRenderingSystemComponent->m_device->CreateRenderTargetView(g_DXRenderingSystemComponent->m_backBufferRenderTarget[1], NULL, g_DXRenderingSystemComponent->m_renderTargetViewHandle);

	return true;
}

bool DX12RenderingSystemNS::createRenderPass()
{
	auto l_DXSPC = addDX12ShaderProgramComponent(m_entityID);

	ShaderFilePaths m_shaderFilePaths = { "DX12//finalBlendPassVertex.hlsl" , "", "DX12//finalBlendPassPixel.hlsl" };

	initializeDX12ShaderProgramComponent(l_DXSPC, m_shaderFilePaths);

	auto l_DXRPC = addDX12RenderPassComponent(m_entityID);

	// Create an empty root signature.
	l_DXRPC->m_rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	l_DXRPC->m_rootSignatureDesc.Desc_1_1.NumParameters = 0;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.pParameters = nullptr;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return true;
}

bool DX12RenderingSystemNS::createCommandList()
{
	HRESULT result;

	// Create a command allocator.
	result = g_DXRenderingSystemComponent->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_commandAllocator));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create command allocator!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Command allocator has been created.");

	// Create a basic command list.
	result = g_DXRenderingSystemComponent->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_DXRenderingSystemComponent->m_commandAllocator, NULL, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_commandList));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create command list!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Command list has been created.");

	// Initially we need to close the command list during initialization as it is created in a recording state.
	result = g_DXRenderingSystemComponent->m_commandList->Close();
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't close the command list!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

bool DX12RenderingSystemNS::createSyncPrimitives()
{
	HRESULT result;

	// Create a fence for GPU synchronization.
	result = g_DXRenderingSystemComponent->m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_fence));
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create fence!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Fence has been created.");

	// Create an event object for the fence.
	g_DXRenderingSystemComponent->m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (g_DXRenderingSystemComponent->m_fenceEvent == NULL)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create fence event!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Fence event has been created.");

	// Initialize the starting fence value.
	g_DXRenderingSystemComponent->m_fenceValue = 1;

	return true;
}

bool DX12RenderingSystemNS::createRasterizer()
{
	// Setup the raster description which will determine how and what polygons will be drawn.
	g_DXRenderingSystemComponent->m_rasterDescForward.AntialiasedLineEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	g_DXRenderingSystemComponent->m_rasterDescForward.CullMode = D3D12_CULL_MODE_NONE;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthBias = 0;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthBiasClamp = 0.0f;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthClipEnable = true;
	g_DXRenderingSystemComponent->m_rasterDescForward.FillMode = D3D12_FILL_MODE_SOLID;
	g_DXRenderingSystemComponent->m_rasterDescForward.FrontCounterClockwise = true;
	g_DXRenderingSystemComponent->m_rasterDescForward.MultisampleEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescForward.SlopeScaledDepthBias = 0.0f;

	g_DXRenderingSystemComponent->m_rasterDescDeferred.AntialiasedLineEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.CullMode = D3D12_CULL_MODE_NONE;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthBias = 0;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthBiasClamp = 0.0f;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthClipEnable = true;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.FillMode = D3D12_FILL_MODE_SOLID;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.FrontCounterClockwise = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.MultisampleEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.SlopeScaledDepthBias = 0.0f;

	// Setup the viewport for rendering.
	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	g_DXRenderingSystemComponent->m_viewport.Width =
		(float)l_screenResolution.x;
	g_DXRenderingSystemComponent->m_viewport.Height =
		(float)l_screenResolution.y;
	g_DXRenderingSystemComponent->m_viewport.MinDepth = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.MaxDepth = 1.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftX = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftY = 0.0f;

	return true;
}

bool DX12RenderingSystemNS::setup(IRenderingFrontendSystem* renderingFrontend)
{
	m_entityID = InnoMath::createEntityID();

	m_renderingFrontendSystem = renderingFrontend;

	g_DXRenderingSystemComponent = &DX12RenderingSystemComponent::get();

	bool result = true;
	result = result && initializeComponentPool();

	result = result && createDebugCallback();
	result = result && createPhysicalDevices();

	result = result && createSwapChain();
	result = result && createBackBuffer();
	result = result && createRenderPass();

	result = result && createCommandList();
	result = result && createSyncPrimitives();
	result = result && createRasterizer();

	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	// Setup the description of the deferred pass.
	g_DXRenderingSystemComponent->deferredPassTextureDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.usageType = TextureUsageType::RENDER_TARGET;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.colorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.width = l_screenResolution.x;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.height = l_screenResolution.y;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT;

	g_DXRenderingSystemComponent->deferredPassRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	g_DXRenderingSystemComponent->deferredPassRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	g_DXRenderingSystemComponent->deferredPassRTVDesc.Texture2D.MipSlice = 0;

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem setup finished.");
	return result;
}

bool DX12RenderingSystemNS::update()
{
	if (m_renderingFrontendSystem->anyUninitializedMeshDataComponent())
	{
		auto l_MDC = m_renderingFrontendSystem->acquireUninitializedMeshDataComponent();
		if (l_MDC)
		{
			auto l_result = generateDX12MeshDataComponent(l_MDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create DXMeshDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}
	if (m_renderingFrontendSystem->anyUninitializedTextureDataComponent())
	{
		auto l_TDC = m_renderingFrontendSystem->acquireUninitializedTextureDataComponent();
		if (l_TDC)
		{
			auto l_result = generateDX12TextureDataComponent(l_TDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create DXTextureDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}

	// Clear the buffers to begin the scene.
	prepareRenderingData();

	//DXGeometryRenderingPassUtilities::update();

	//DXLightRenderingPassUtilities::update();

	//DXFinalRenderingPassUtilities::update();

	return true;
}

bool DX12RenderingSystemNS::terminate()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->SetFullscreenState(false, NULL);
	}

	// Close the object handle to the fence event.
	auto error = CloseHandle(g_DXRenderingSystemComponent->m_fenceEvent);
	if (error == 0)
	{
	}

	// Release the fence.
	if (g_DXRenderingSystemComponent->m_fence)
	{
		g_DXRenderingSystemComponent->m_fence->Release();
		g_DXRenderingSystemComponent->m_fence = 0;
	}

	// Release the command list.
	if (g_DXRenderingSystemComponent->m_commandList)
	{
		g_DXRenderingSystemComponent->m_commandList->Release();
		g_DXRenderingSystemComponent->m_commandList = 0;
	}

	// Release the command allocator.
	if (g_DXRenderingSystemComponent->m_commandAllocator)
	{
		g_DXRenderingSystemComponent->m_commandAllocator->Release();
		g_DXRenderingSystemComponent->m_commandAllocator = 0;
	}

	// Release the back buffer render target views.
	if (g_DXRenderingSystemComponent->m_backBufferRenderTarget[0])
	{
		g_DXRenderingSystemComponent->m_backBufferRenderTarget[0]->Release();
		g_DXRenderingSystemComponent->m_backBufferRenderTarget[0] = 0;
	}
	if (g_DXRenderingSystemComponent->m_backBufferRenderTarget[1])
	{
		g_DXRenderingSystemComponent->m_backBufferRenderTarget[1]->Release();
		g_DXRenderingSystemComponent->m_backBufferRenderTarget[1] = 0;
	}

	// Release the render target view heap.
	if (g_DXRenderingSystemComponent->m_renderTargetViewHeap)
	{
		g_DXRenderingSystemComponent->m_renderTargetViewHeap->Release();
		g_DXRenderingSystemComponent->m_renderTargetViewHeap = 0;
	}

	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->Release();
		g_DXRenderingSystemComponent->m_swapChain = 0;
	}

	// Release the command queue.
	if (g_DXRenderingSystemComponent->m_commandQueue)
	{
		g_DXRenderingSystemComponent->m_commandQueue->Release();
		g_DXRenderingSystemComponent->m_commandQueue = 0;
	}

	if (g_DXRenderingSystemComponent->m_device)
	{
		g_DXRenderingSystemComponent->m_device->Release();
		g_DXRenderingSystemComponent->m_device = 0;
	}

	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem has been terminated.");
	return true;
}

bool  DX12RenderingSystemNS::initializeDefaultAssets()
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::LINE);
	g_DXRenderingSystemComponent->m_UnitLineDXMDC = generateDX12MeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	g_DXRenderingSystemComponent->m_UnitQuadDXMDC = generateDX12MeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
	g_DXRenderingSystemComponent->m_UnitCubeDXMDC = generateDX12MeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE);
	g_DXRenderingSystemComponent->m_UnitSphereDXMDC = generateDX12MeshDataComponent(l_MDC);

	g_DXRenderingSystemComponent->m_basicNormalDXTDC = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::NORMAL));
	g_DXRenderingSystemComponent->m_basicAlbedoDXTDC = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO));
	g_DXRenderingSystemComponent->m_basicMetallicDXTDC = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::METALLIC));
	g_DXRenderingSystemComponent->m_basicRoughnessDXTDC = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	g_DXRenderingSystemComponent->m_basicAODXTDC = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));

	g_DXRenderingSystemComponent->m_iconTemplate_OBJ = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::OBJ));
	g_DXRenderingSystemComponent->m_iconTemplate_PNG = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::PNG));
	g_DXRenderingSystemComponent->m_iconTemplate_SHADER = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::SHADER));
	g_DXRenderingSystemComponent->m_iconTemplate_UNKNOWN = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::UNKNOWN));

	g_DXRenderingSystemComponent->m_iconTemplate_DirectionalLight = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::DIRECTIONAL_LIGHT));
	g_DXRenderingSystemComponent->m_iconTemplate_PointLight = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::POINT_LIGHT));
	g_DXRenderingSystemComponent->m_iconTemplate_SphereLight = generateDX12TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::SPHERE_LIGHT));

	return true;
}

void DX12RenderingSystemNS::prepareRenderingData()
{
	// copy camera data pack for local scope
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	if (l_cameraDataPack.has_value())
	{
		m_cameraDataPack = l_cameraDataPack.value();
	}

	// copy sun data pack for local scope
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();
	if (l_sunDataPack.has_value())
	{
		m_sunDataPack = l_sunDataPack.value();
	}

	g_DXRenderingSystemComponent->m_cameraCBufferData.p_original = m_cameraDataPack.p_original;
	g_DXRenderingSystemComponent->m_cameraCBufferData.p_jittered = m_cameraDataPack.p_jittered;
	g_DXRenderingSystemComponent->m_cameraCBufferData.r = m_cameraDataPack.r;
	g_DXRenderingSystemComponent->m_cameraCBufferData.t = m_cameraDataPack.t;
	g_DXRenderingSystemComponent->m_cameraCBufferData.r_prev = m_cameraDataPack.r_prev;
	g_DXRenderingSystemComponent->m_cameraCBufferData.t_prev = m_cameraDataPack.t_prev;
	g_DXRenderingSystemComponent->m_cameraCBufferData.globalPos = m_cameraDataPack.globalPos;

	g_DXRenderingSystemComponent->m_directionalLightCBufferData.dir = m_sunDataPack.dir;
	g_DXRenderingSystemComponent->m_directionalLightCBufferData.luminance = m_sunDataPack.luminance;

	auto l_meshDataPack = m_renderingFrontendSystem->getMeshDataPack();

	if (l_meshDataPack.has_value())
	{
		m_meshDataPack = l_meshDataPack.value();
	}

	for (auto i : m_meshDataPack)
	{
		auto l_DXMDC = getDX12MeshDataComponent(i.MDC->m_parentEntity);
		if (l_DXMDC && l_DXMDC->m_objectStatus == ObjectStatus::ALIVE)
		{
			DX12MeshDataPack l_meshDataPack;

			l_meshDataPack.indiceSize = i.MDC->m_indicesSize;
			l_meshDataPack.meshPrimitiveTopology = i.MDC->m_meshPrimitiveTopology;
			l_meshDataPack.meshCBuffer.m = i.m;
			l_meshDataPack.meshCBuffer.m_prev = i.m_prev;
			l_meshDataPack.meshCBuffer.normalMat = i.normalMat;
			l_meshDataPack.DXMDC = l_DXMDC;

			auto l_material = i.material;
			// any normal?
			auto l_TDC = l_material->m_texturePack.m_normalTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.normalDXTDC = getDX12TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useNormalTexture = false;
			}
			// any albedo?
			l_TDC = l_material->m_texturePack.m_albedoTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.albedoDXTDC = getDX12TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useAlbedoTexture = false;
			}
			// any metallic?
			l_TDC = l_material->m_texturePack.m_metallicTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.metallicDXTDC = getDX12TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useMetallicTexture = false;
			}
			// any roughness?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.roughnessDXTDC = getDX12TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useRoughnessTexture = false;
			}
			// any ao?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.AODXTDC = getDX12TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useAOTexture = false;
			}

			l_meshDataPack.textureCBuffer.albedo = vec4(
				l_material->m_meshCustomMaterial.albedo_r,
				l_material->m_meshCustomMaterial.albedo_g,
				l_material->m_meshCustomMaterial.albedo_b,
				1.0f
			);
			l_meshDataPack.textureCBuffer.MRA = vec4(
				l_material->m_meshCustomMaterial.metallic,
				l_material->m_meshCustomMaterial.roughness,
				l_material->m_meshCustomMaterial.ao,
				1.0f
			);

			g_DXRenderingSystemComponent->m_meshDataQueue.push(l_meshDataPack);
		}
	}
}

bool DX12RenderingSystem::setup(IRenderingFrontendSystem* renderingFrontend)
{
	return DX12RenderingSystemNS::setup(renderingFrontend);
}

bool DX12RenderingSystem::initialize()
{
	DX12RenderingSystemNS::initializeDefaultAssets();
	//DXGeometryRenderingPassUtilities::initialize();
	//DXLightRenderingPassUtilities::initialize();
	//DXFinalRenderingPassUtilities::initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem has been initialized.");
	return true;
}

bool DX12RenderingSystem::update()
{
	return DX12RenderingSystemNS::update();
}

bool DX12RenderingSystem::terminate()
{
	return DX12RenderingSystemNS::terminate();
}

ObjectStatus DX12RenderingSystem::getStatus()
{
	return DX12RenderingSystemNS::m_objectStatus;
}

bool DX12RenderingSystem::resize()
{
	return true;
}

bool DX12RenderingSystem::reloadShader(RenderPassType renderPassType)
{
	return true;
}

bool DX12RenderingSystem::bakeGI()
{
	return true;
}
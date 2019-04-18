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

	static DX12RenderingSystemComponent* g_DXRenderingSystemComponent;

	ThreadSafeUnorderedMap<EntityID, DX12MeshDataComponent*> m_meshMap;
	ThreadSafeUnorderedMap<EntityID, MaterialDataComponent*> m_materialMap;
	ThreadSafeUnorderedMap<EntityID, DX12TextureDataComponent*> m_textureMap;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<DX12MeshDataComponent*> m_uninitializedMDC;
	ThreadSafeQueue<DX12TextureDataComponent*> m_uninitializedTDC;

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

	DX12TextureDataComponent* m_basicNormalTDC;
	DX12TextureDataComponent* m_basicAlbedoTDC;
	DX12TextureDataComponent* m_basicMetallicTDC;
	DX12TextureDataComponent* m_basicRoughnessTDC;
	DX12TextureDataComponent* m_basicAOTDC;
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

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

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

	auto l_result = createRootSignature(l_DXRPC);
	l_result = createPSO(l_DXRPC, l_DXSPC);

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
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

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

bool DX12RenderingSystemNS::setup()
{
	m_entityID = InnoMath::createEntityID();

	g_DXRenderingSystemComponent = &DX12RenderingSystemComponent::get();

	bool result = true;
	result = result && initializeComponentPool();

	result = result && createDebugCallback();
	result = result && createPhysicalDevices();

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

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

bool DX12RenderingSystemNS::initialize()
{
	m_MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX12MeshDataComponent), 16384);
	m_MaterialDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MaterialDataComponent), 32768);
	m_TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX12TextureDataComponent), 32768);

	bool result = true;

	result = result && createSwapChain();
	result = result && createBackBuffer();
	result = result && createRenderPass();

	result = result && createCommandList();
	result = result && createSyncPrimitives();
	result = result && createRasterizer();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem has been initialized.");

	return result;
}

bool DX12RenderingSystemNS::update()
{
	return true;
}

bool DX12RenderingSystemNS::render()
{
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

void DX12RenderingSystemNS::loadDefaultAssets()
{
	auto l_basicNormalTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_basicAlbedoTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	auto l_basicMetallicTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	auto l_basicRoughnessTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	auto l_basicAOTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	auto l_iconTemplate_OBJ = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PNG = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SHADER = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_UNKNOWN = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	auto l_iconTemplate_DirectionalLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PointLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SphereLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_basicNormalTDC = reinterpret_cast<DX12TextureDataComponent*>(l_basicNormalTDC);
	m_basicAlbedoTDC = reinterpret_cast<DX12TextureDataComponent*>(l_basicAlbedoTDC);
	m_basicMetallicTDC = reinterpret_cast<DX12TextureDataComponent*>(l_basicMetallicTDC);
	m_basicRoughnessTDC = reinterpret_cast<DX12TextureDataComponent*>(l_basicRoughnessTDC);
	m_basicAOTDC = reinterpret_cast<DX12TextureDataComponent*>(l_basicAOTDC);

	m_iconTemplate_OBJ = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_OBJ);
	m_iconTemplate_PNG = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_PNG);
	m_iconTemplate_SHADER = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_SHADER);
	m_iconTemplate_UNKNOWN = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_UNKNOWN);

	m_iconTemplate_DirectionalLight = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_DirectionalLight);
	m_iconTemplate_PointLight = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_PointLight);
	m_iconTemplate_SphereLight = reinterpret_cast<DX12TextureDataComponent*>(l_iconTemplate_SphereLight);

	m_unitLineMDC = addDX12MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addDX12MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitQuad(*m_unitQuadMDC);
	// Flip y texture coordinate
	for (auto& i : m_unitQuadMDC->m_vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}
	m_unitQuadMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitQuadMDC->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMDC);

	m_unitCubeMDC = addDX12MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addDX12MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addDX12MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

	initializeDX12MeshDataComponent(m_unitLineMDC);
	initializeDX12MeshDataComponent(m_unitQuadMDC);
	initializeDX12MeshDataComponent(m_unitCubeMDC);
	initializeDX12MeshDataComponent(m_unitSphereMDC);
	initializeDX12MeshDataComponent(m_terrainMDC);

	initializeDX12TextureDataComponent(m_basicNormalTDC);
	initializeDX12TextureDataComponent(m_basicAlbedoTDC);
	initializeDX12TextureDataComponent(m_basicMetallicTDC);
	initializeDX12TextureDataComponent(m_basicRoughnessTDC);
	initializeDX12TextureDataComponent(m_basicAOTDC);

	initializeDX12TextureDataComponent(m_iconTemplate_OBJ);
	initializeDX12TextureDataComponent(m_iconTemplate_PNG);
	initializeDX12TextureDataComponent(m_iconTemplate_SHADER);
	initializeDX12TextureDataComponent(m_iconTemplate_UNKNOWN);

	initializeDX12TextureDataComponent(m_iconTemplate_DirectionalLight);
	initializeDX12TextureDataComponent(m_iconTemplate_PointLight);
	initializeDX12TextureDataComponent(m_iconTemplate_SphereLight);
}

DX12MeshDataComponent* DX12RenderingSystemNS::addDX12MeshDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(DX12MeshDataComponent));
	auto l_MDC = new(l_rawPtr)DX12MeshDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, DX12MeshDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

MaterialDataComponent* DX12RenderingSystemNS::addMaterialDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(MaterialDataComponent));
	auto l_MDC = new(l_rawPtr)MaterialDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_materialMap = &m_materialMap;
	l_materialMap->emplace(std::pair<EntityID, MaterialDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

DX12TextureDataComponent* DX12RenderingSystemNS::addDX12TextureDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(DX12TextureDataComponent));
	auto l_TDC = new(l_rawPtr)DX12TextureDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_TDC->m_parentEntity = l_parentEntity;
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, DX12TextureDataComponent*>(l_parentEntity, l_TDC));
	return l_TDC;
}

DX12MeshDataComponent* DX12RenderingSystemNS::getDX12MeshDataComponent(EntityID EntityID)
{
	auto result = DX12RenderingSystemNS::m_meshMap.find(EntityID);
	if (result != DX12RenderingSystemNS::m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't find MeshDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

DX12TextureDataComponent * DX12RenderingSystemNS::getDX12TextureDataComponent(EntityID EntityID)
{
	auto result = DX12RenderingSystemNS::m_textureMap.find(EntityID);
	if (result != DX12RenderingSystemNS::m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't find TextureDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

DX12MeshDataComponent* DX12RenderingSystemNS::getDX12MeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return DX12RenderingSystemNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return DX12RenderingSystemNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return DX12RenderingSystemNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return DX12RenderingSystemNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return DX12RenderingSystemNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: wrong MeshShapeType passed to DX12RenderingSystem::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

DX12TextureDataComponent * DX12RenderingSystemNS::getDX12TextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return DX12RenderingSystemNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return DX12RenderingSystemNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return DX12RenderingSystemNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return DX12RenderingSystemNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return DX12RenderingSystemNS::m_basicAOTDC; break;
	case TextureUsageType::RENDER_TARGET:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

DX12TextureDataComponent * DX12RenderingSystemNS::getDX12TextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return DX12RenderingSystemNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return DX12RenderingSystemNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return DX12RenderingSystemNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return DX12RenderingSystemNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

DX12TextureDataComponent * DX12RenderingSystemNS::getDX12TextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return DX12RenderingSystemNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return DX12RenderingSystemNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return DX12RenderingSystemNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool DX12RenderingSystemNS::resize()
{
	return true;
}

bool DX12RenderingSystem::setup()
{
	return DX12RenderingSystemNS::setup();
}

bool DX12RenderingSystem::initialize()
{
	return DX12RenderingSystemNS::initialize();
}

bool DX12RenderingSystem::update()
{
	return DX12RenderingSystemNS::update();
}

bool DX12RenderingSystem::render()
{
	return DX12RenderingSystemNS::render();
}

bool DX12RenderingSystem::terminate()
{
	return DX12RenderingSystemNS::terminate();
}

ObjectStatus DX12RenderingSystem::getStatus()
{
	return DX12RenderingSystemNS::m_objectStatus;
}

MeshDataComponent * DX12RenderingSystem::addMeshDataComponent()
{
	return DX12RenderingSystemNS::addDX12MeshDataComponent();
}

MaterialDataComponent * DX12RenderingSystem::addMaterialDataComponent()
{
	return DX12RenderingSystemNS::addMaterialDataComponent();
}

TextureDataComponent * DX12RenderingSystem::addTextureDataComponent()
{
	return DX12RenderingSystemNS::addDX12TextureDataComponent();
}

MeshDataComponent * DX12RenderingSystem::getMeshDataComponent(EntityID meshID)
{
	return DX12RenderingSystemNS::getDX12MeshDataComponent(meshID);
}

TextureDataComponent * DX12RenderingSystem::getTextureDataComponent(EntityID textureID)
{
	return DX12RenderingSystemNS::getDX12TextureDataComponent(textureID);
}

MeshDataComponent * DX12RenderingSystem::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return DX12RenderingSystemNS::getDX12MeshDataComponent(MeshShapeType);
}

TextureDataComponent * DX12RenderingSystem::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return DX12RenderingSystemNS::getDX12TextureDataComponent(TextureUsageType);
}

TextureDataComponent * DX12RenderingSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	return DX12RenderingSystemNS::getDX12TextureDataComponent(iconType);
}

TextureDataComponent * DX12RenderingSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	return DX12RenderingSystemNS::getDX12TextureDataComponent(iconType);
}

bool DX12RenderingSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &DX12RenderingSystemNS::m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		g_pCoreSystem->getMemorySystem()->destroyObject(DX12RenderingSystemNS::m_MeshDataComponentPool, sizeof(DX12MeshDataComponent), l_mesh->second);
		l_meshMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't remove MeshDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool DX12RenderingSystem::removeTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &DX12RenderingSystemNS::m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO
		}

		g_pCoreSystem->getMemorySystem()->destroyObject(DX12RenderingSystemNS::m_TextureDataComponentPool, sizeof(DX12TextureDataComponent), l_texture->second);
		l_textureMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't remove TextureDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

void DX12RenderingSystem::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	DX12RenderingSystemNS::m_uninitializedMDC.push(reinterpret_cast<DX12MeshDataComponent*>(rhs));
}

void DX12RenderingSystem::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	DX12RenderingSystemNS::m_uninitializedTDC.push(reinterpret_cast<DX12TextureDataComponent*>(rhs));
}

bool DX12RenderingSystem::resize()
{
	return DX12RenderingSystemNS::resize();
}

bool DX12RenderingSystem::reloadShader(RenderPassType renderPassType)
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

bool DX12RenderingSystem::bakeGI()
{
	return true;
}
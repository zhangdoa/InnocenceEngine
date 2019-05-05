#include "DX12RenderingSystem.h"

#include "DX12RenderingSystemUtilities.h"

#include "DX12OpaquePass.h"

#include "../../component/DX12RenderingSystemComponent.h"
#include "../../component/WinWindowSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

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

	bool createDebugCallback();
	bool createPhysicalDevices();
	bool createCommandAllocator();

	bool createCSUHeap();
	bool createSwapChain();
	bool createSwapChainDXRPC();
	bool createSwapChainCommandLists();
	bool createSwapChainSyncPrimitives();

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
	HRESULT l_result;

	// Create a DirectX graphics interface factory.
	l_result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_factory));
	if (FAILED(l_result))
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
	l_result = D3D12CreateDevice(g_DXRenderingSystemComponent->m_adapter, featureLevel, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_device));
	if (FAILED(l_result))
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
	l_result = g_DXRenderingSystemComponent->m_device->CreateCommandQueue(&g_DXRenderingSystemComponent->m_commandQueueDesc, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_commandQueue));
	if (FAILED(l_result))
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

bool DX12RenderingSystemNS::createCommandAllocator()
{
	HRESULT l_result;

	// Create a command allocator.
	l_result = g_DXRenderingSystemComponent->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_commandAllocator));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create command allocator!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Command allocator has been created.");

	return true;
}

bool DX12RenderingSystemNS::createCSUHeap()
{
	// @TODO: one render pass component has one CSU heap, decouple CBV from DX12ConstantBuffer
	g_DXRenderingSystemComponent->m_CSUHeapDesc.NumDescriptors = 65536;
	g_DXRenderingSystemComponent->m_CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	g_DXRenderingSystemComponent->m_CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = DX12RenderingSystemComponent::get().m_device->CreateDescriptorHeap(&g_DXRenderingSystemComponent->m_CSUHeapDesc, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_CSUHeap));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create DescriptorHeap for CBV/SRV/UAV!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_CSUHeap->SetName(L"GlobalCSUHeap");

	g_DXRenderingSystemComponent->m_initialCSUCPUHandle = g_DXRenderingSystemComponent->m_CSUHeap->GetCPUDescriptorHandleForHeapStart();
	g_DXRenderingSystemComponent->m_initialCSUGPUHandle = g_DXRenderingSystemComponent->m_CSUHeap->GetGPUDescriptorHandleForHeapStart();

	g_DXRenderingSystemComponent->m_currentCSUCPUHandle = g_DXRenderingSystemComponent->m_initialCSUCPUHandle;
	g_DXRenderingSystemComponent->m_currentCSUGPUHandle = g_DXRenderingSystemComponent->m_initialCSUGPUHandle;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: DescriptorHeap for CBV/SRV/UAV has been created.");

	return true;
}

bool DX12RenderingSystemNS::createSwapChain()
{
	HRESULT l_result;

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
	l_result = g_DXRenderingSystemComponent->m_factory->CreateSwapChainForHwnd(
		g_DXRenderingSystemComponent->m_commandQueue,
		WinWindowSystemComponent::get().m_hwnd,
		&g_DXRenderingSystemComponent->m_swapChainDesc,
		nullptr,
		nullptr,
		&l_swapChain1);

	g_DXRenderingSystemComponent->m_swapChain = reinterpret_cast<IDXGISwapChain4*>(l_swapChain1);

	if (FAILED(l_result))
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

bool DX12RenderingSystemNS::createSwapChainDXRPC()
{
	auto l_imageCount = 2;

	auto l_DXSPC = addDX12ShaderProgramComponent(m_entityID);

	ShaderFilePaths m_shaderFilePaths = { "DX12//finalBlendPassVertex.hlsl" , "", "DX12//finalBlendPassPixel.hlsl" };

	initializeDX12ShaderProgramComponent(l_DXSPC, m_shaderFilePaths);

	auto l_DXRPC = addDX12RenderPassComponent(m_entityID);

	l_DXRPC->m_renderPassDesc = DX12RenderingSystemComponent::get().m_deferredRenderPassDesc;
	l_DXRPC->m_renderPassDesc.RTNumber = l_imageCount;
	l_DXRPC->m_renderPassDesc.useMultipleFramebuffers = true;

	l_DXRPC->m_RTVHeapDesc.NumDescriptors = l_imageCount;
	l_DXRPC->m_RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	l_DXRPC->m_RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// initialize manually
	bool l_result = true;

	l_result &= reserveRenderTargets(l_DXRPC);

	l_result &= createRTVDescriptorHeap(l_DXRPC);

	auto l_RTVDescSize = g_DXRenderingSystemComponent->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE l_handle = l_DXRPC->m_RTVDescHandle;

	// use device created swap chain RTV
	for (size_t i = 0; i < l_imageCount; i++)
	{
		auto l_result = g_DXRenderingSystemComponent->m_swapChain->GetBuffer((unsigned int)i, IID_PPV_ARGS(&l_DXRPC->m_DXTDCs[i]->m_texture));
		if (FAILED(l_result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't get pointer of swap chain render target " + std::to_string(i) + "!");
			m_objectStatus = ObjectStatus::STANDBY;
			return false;
		}
		g_DXRenderingSystemComponent->m_device->CreateRenderTargetView(l_DXRPC->m_DXTDCs[i]->m_texture, NULL, l_handle);
		l_handle.ptr += l_RTVDescSize;
		l_DXRPC->m_DXTDCs[i]->m_DX12TextureDataDesc = l_DXRPC->m_DXTDCs[i]->m_texture->GetDesc();
	}

	// Create an empty root signature.
	l_DXRPC->m_rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	l_DXRPC->m_rootSignatureDesc.Desc_1_1.NumParameters = 0;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.pParameters = nullptr;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	l_DXRPC->m_rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	l_DXRPC->m_blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// Set up the depth stencil view description.
	l_DXRPC->m_depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	l_DXRPC->m_depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	l_DXRPC->m_depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Setup the viewport for rendering.
	l_DXRPC->m_viewport.Width = (float)l_DXRPC->m_renderPassDesc.RTDesc.width;
	l_DXRPC->m_viewport.Height = (float)l_DXRPC->m_renderPassDesc.RTDesc.height;
	l_DXRPC->m_viewport.MinDepth = 0.0f;
	l_DXRPC->m_viewport.MaxDepth = 1.0f;
	l_DXRPC->m_viewport.TopLeftX = 0.0f;
	l_DXRPC->m_viewport.TopLeftY = 0.0f;

	// Setup the scissor rect
	l_DXRPC->m_scissor.left = 0;
	l_DXRPC->m_scissor.top = 0;
	l_DXRPC->m_scissor.right = (unsigned long)l_DXRPC->m_viewport.Width;
	l_DXRPC->m_scissor.bottom = (unsigned long)l_DXRPC->m_viewport.Height;

	// Describe and create the graphics pipeline state object (PSO).
	l_DXRPC->m_PSODesc.RasterizerState = l_DXRPC->m_rasterizerDesc;
	l_DXRPC->m_PSODesc.BlendState = l_DXRPC->m_blendDesc;
	l_DXRPC->m_PSODesc.DepthStencilState.DepthEnable = false;
	l_DXRPC->m_PSODesc.DepthStencilState.StencilEnable = false;
	l_DXRPC->m_PSODesc.SampleMask = UINT_MAX;
	l_DXRPC->m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	l_DXRPC->m_PSODesc.SampleDesc.Count = 1;

	l_result = createRootSignature(l_DXRPC);
	l_result = createPSO(l_DXRPC, l_DXSPC);
	l_result = createCommandLists(l_DXRPC);

	DX12RenderingSystemComponent::get().m_swapChainDXRPC = l_DXRPC;
	DX12RenderingSystemComponent::get().m_swapChainDXSPC = l_DXSPC;

	return true;
}

bool DX12RenderingSystemNS::createSwapChainCommandLists()
{
	HRESULT l_result;
	l_result = DX12RenderingSystemComponent::get().m_commandAllocator->Reset();

	auto l_MDC = getDX12MeshDataComponent(MeshShapeType::QUAD);

	for (size_t i = 0; i < DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_commandLists.size(); i++)
	{
		auto l_commandIndex = (unsigned int)i;
		recordCommandBegin(DX12RenderingSystemComponent::get().m_swapChainDXRPC, l_commandIndex);

		DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_commandLists[l_commandIndex]->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_DXTDCs[l_commandIndex]->m_texture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		recordActivateRenderPass(DX12RenderingSystemComponent::get().m_swapChainDXRPC, l_commandIndex);

		recordDrawCall(DX12RenderingSystemComponent::get().m_swapChainDXRPC, l_commandIndex, l_MDC);

		DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_commandLists[l_commandIndex]->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_DXTDCs[l_commandIndex]->m_texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		recordCommandEnd(DX12RenderingSystemComponent::get().m_swapChainDXRPC, l_commandIndex);
	}

	return true;
}

bool DX12RenderingSystemNS::createSwapChainSyncPrimitives()
{
	return 	createSyncPrimitives(DX12RenderingSystemComponent::get().m_swapChainDXRPC);
}

bool DX12RenderingSystemNS::setup()
{
	m_entityID = InnoMath::createEntityID();

	g_DXRenderingSystemComponent = &DX12RenderingSystemComponent::get();

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	// general render pass desc
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTNumber = 1;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.width = l_screenResolution.x;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.height = l_screenResolution.y;
	g_DXRenderingSystemComponent->m_deferredRenderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;

	bool l_result = true;
	l_result = l_result && initializeComponentPool();

	l_result = l_result && createDebugCallback();
	l_result = l_result && createPhysicalDevices();

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem setup finished.");
	return l_result;
}

bool DX12RenderingSystemNS::initialize()
{
	m_MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX12MeshDataComponent), RenderingFrontendSystemComponent::get().m_maxMeshes);
	m_MaterialDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MaterialDataComponent), RenderingFrontendSystemComponent::get().m_maxMaterials);
	m_TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX12TextureDataComponent), RenderingFrontendSystemComponent::get().m_maxTextures);

	bool l_result = true;

	l_result = l_result && createCommandAllocator();

	l_result = l_result && createCSUHeap();

	loadDefaultAssets();

	generateGPUBuffers();

	l_result = l_result && createSwapChain();
	l_result = l_result && createSwapChainDXRPC();
	l_result = l_result && createSwapChainCommandLists();

	l_result = l_result && createSwapChainSyncPrimitives();

	DX12OpaquePass::initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem has been initialized.");

	return l_result;
}

bool DX12RenderingSystemNS::update()
{
	updateConstantBuffer(DX12RenderingSystemComponent::get().m_cameraConstantBuffer, RenderingFrontendSystemComponent::get().m_cameraGPUData);
	// @TODO: prepare in rendering frontend
	auto l_queueCopy = RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.getRawData();

	if (l_queueCopy.size() > 0)
	{
		std::vector<MeshGPUData> l_meshGPUData;
		l_meshGPUData.reserve(l_queueCopy.size());

		std::vector<MaterialGPUData> l_materialGPUData;
		l_materialGPUData.reserve(l_queueCopy.size());

		while (l_queueCopy.size() > 0)
		{
			auto l_geometryPassGPUData = l_queueCopy.front();
			l_meshGPUData.emplace_back(l_geometryPassGPUData.meshGPUData);
			l_materialGPUData.emplace_back(l_geometryPassGPUData.materialGPUData);
			l_queueCopy.pop();
		}
		updateConstantBuffer(DX12RenderingSystemComponent::get().m_meshConstantBuffer, l_meshGPUData);
		updateConstantBuffer(DX12RenderingSystemComponent::get().m_materialConstantBuffer, l_materialGPUData);
	}

	DX12OpaquePass::update();

	return true;
}

bool DX12RenderingSystemNS::render()
{
	DX12OpaquePass::render();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_commandLists[DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_frameIndex] };
	DX12RenderingSystemComponent::get().m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	DX12RenderingSystemComponent::get().m_swapChain->Present(1, 0);

	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fenceValues[DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_frameIndex];
	DX12RenderingSystemComponent::get().m_commandQueue->Signal(DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fence, currentFenceValue);

	// Update the frame index.
	DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_frameIndex = DX12RenderingSystemComponent::get().m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fence->GetCompletedValue() < DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fenceValues[DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_frameIndex])
	{
		DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fence->SetEventOnCompletion(DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fenceValues[DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_frameIndex], DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fenceEvent);
		WaitForSingleObjectEx(DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fenceValues[DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_frameIndex] = currentFenceValue + 1;

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
	auto error = CloseHandle(DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fenceEvent);
	if (error == 0)
	{
	}

	// Release the fence.
	if (DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fence)
	{
		DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fence->Release();
		DX12RenderingSystemComponent::get().m_swapChainDXRPC->m_fence = 0;
	}

	// Release the command allocator.
	if (g_DXRenderingSystemComponent->m_commandAllocator)
	{
		g_DXRenderingSystemComponent->m_commandAllocator->Release();
		g_DXRenderingSystemComponent->m_commandAllocator = 0;
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
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
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

bool DX12RenderingSystemNS::generateGPUBuffers()
{
	g_DXRenderingSystemComponent->m_cameraConstantBuffer = createConstantBuffer(sizeof(CameraGPUData), 1, L"cameraConstantBuffer");
	g_DXRenderingSystemComponent->m_meshConstantBuffer = createConstantBuffer(sizeof(MeshGPUData), RenderingFrontendSystemComponent::get().m_maxMeshes, L"meshConstantBuffer");
	g_DXRenderingSystemComponent->m_materialConstantBuffer = createConstantBuffer(sizeof(MaterialGPUData), RenderingFrontendSystemComponent::get().m_maxMaterials, L"materialConstantBuffer");

	return true;
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
	auto l_result = DX12RenderingSystemNS::m_meshMap.find(EntityID);
	if (l_result != DX12RenderingSystemNS::m_meshMap.end())
	{
		return l_result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't find MeshDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

DX12TextureDataComponent * DX12RenderingSystemNS::getDX12TextureDataComponent(EntityID EntityID)
{
	auto l_result = DX12RenderingSystemNS::m_textureMap.find(EntityID);
	if (l_result != DX12RenderingSystemNS::m_textureMap.end())
	{
		return l_result->second;
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
	case TextureUsageType::COLOR_ATTACHMENT:
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
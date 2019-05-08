#include "DX12RenderingSystem.h"

#include "DX12RenderingSystemUtilities.h"

#include "DX12OpaquePass.h"
#include "DX12LightPass.h"

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
	bool createGlobalCommandAllocator();

	bool createGlobalCSUHeap();
	bool createGlobalSamplerHeap();
	bool createSwapChain();
	bool createSwapChainDXRPC();
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
	ZeroMemory(&g_DXRenderingSystemComponent->m_globalCommandQueueDesc, sizeof(g_DXRenderingSystemComponent->m_globalCommandQueueDesc));

	// Set up the description of the command queue.
	g_DXRenderingSystemComponent->m_globalCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	g_DXRenderingSystemComponent->m_globalCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	g_DXRenderingSystemComponent->m_globalCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	g_DXRenderingSystemComponent->m_globalCommandQueueDesc.NodeMask = 0;

	// Create the command queue.
	l_result = g_DXRenderingSystemComponent->m_device->CreateCommandQueue(&g_DXRenderingSystemComponent->m_globalCommandQueueDesc, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_globalCommandQueue));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create global CommandQueue!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_globalCommandQueue->SetName(L"GlobalCommandQueue");

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Global CommandQueue has been created.");

	// Release the adapter.
	g_DXRenderingSystemComponent->m_adapter->Release();
	g_DXRenderingSystemComponent->m_adapter = 0;

	return true;
}

bool DX12RenderingSystemNS::createGlobalCommandAllocator()
{
	HRESULT l_result;

	// Create a command allocator.
	l_result = g_DXRenderingSystemComponent->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_globalCommandAllocator));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create global CommandAllocator!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_globalCommandAllocator->SetName(L"GlobalCommandAllocator");

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Global CommandAllocator has been created.");

	return true;
}

bool DX12RenderingSystemNS::createGlobalCSUHeap()
{
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

bool DX12RenderingSystemNS::createGlobalSamplerHeap()
{
	g_DXRenderingSystemComponent->m_samplerHeapDesc.NumDescriptors = 128;
	g_DXRenderingSystemComponent->m_samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	g_DXRenderingSystemComponent->m_samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = DX12RenderingSystemComponent::get().m_device->CreateDescriptorHeap(&g_DXRenderingSystemComponent->m_samplerHeapDesc, IID_PPV_ARGS(&g_DXRenderingSystemComponent->m_samplerHeap));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create DescriptorHeap for Sampler!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_samplerHeap->SetName(L"GlobalSamplerHeap");

	g_DXRenderingSystemComponent->m_initialSamplerCPUHandle = g_DXRenderingSystemComponent->m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
	g_DXRenderingSystemComponent->m_initialSamplerGPUHandle = g_DXRenderingSystemComponent->m_samplerHeap->GetGPUDescriptorHandleForHeapStart();

	g_DXRenderingSystemComponent->m_currentSamplerCPUHandle = g_DXRenderingSystemComponent->m_initialSamplerCPUHandle;
	g_DXRenderingSystemComponent->m_currentSamplerGPUHandle = g_DXRenderingSystemComponent->m_initialSamplerGPUHandle;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: DescriptorHeap for Sampler has been created.");

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
		g_DXRenderingSystemComponent->m_globalCommandQueue,
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

	l_DXSPC->m_samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	l_DXSPC->m_samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	l_DXSPC->m_samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	l_DXSPC->m_samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	l_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	l_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	l_DXSPC->m_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	l_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	l_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	l_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	l_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	l_DXSPC->m_samplerDesc.MinLOD = 0;
	l_DXSPC->m_samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	initializeDX12ShaderProgramComponent(l_DXSPC, m_shaderFilePaths);

	auto l_DXRPC = addDX12RenderPassComponent(m_entityID);

	l_DXRPC->m_renderPassDesc = DX12RenderingSystemComponent::get().m_deferredRenderPassDesc;
	l_DXRPC->m_renderPassDesc.RTNumber = l_imageCount;
	l_DXRPC->m_renderPassDesc.useMultipleFramebuffers = true;

	// Setup the RTV description.
	l_DXRPC->m_RTVHeapDesc.NumDescriptors = l_imageCount;
	l_DXRPC->m_RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	l_DXRPC->m_RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	l_DXRPC->m_RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	l_DXRPC->m_RTVDesc.Texture2D.MipSlice = 0;

	// Setup the DSV description.
	l_DXRPC->m_DSVHeapDesc.NumDescriptors = 1;
	l_DXRPC->m_DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	l_DXRPC->m_DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	l_DXRPC->m_DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	l_DXRPC->m_DSVDesc.Texture2D.MipSlice = 0;

	// initialize manually
	bool l_result = true;
	l_result &= reserveRenderTargets(l_DXRPC);
	l_result &= createRTVDescriptorHeap(l_DXRPC);

	// use device created swap chain textures
	for (size_t i = 0; i < l_imageCount; i++)
	{
		auto l_result = g_DXRenderingSystemComponent->m_swapChain->GetBuffer((unsigned int)i, IID_PPV_ARGS(&l_DXRPC->m_DXTDCs[i]->m_texture));
		if (FAILED(l_result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't get pointer of swap chain render target " + std::to_string(i) + "!");
			m_objectStatus = ObjectStatus::STANDBY;
			return false;
		}
		l_DXRPC->m_DXTDCs[i]->m_DX12TextureDataDesc = l_DXRPC->m_DXTDCs[i]->m_texture->GetDesc();
	}

	l_result &= createRTV(l_DXRPC);

	l_DXRPC->m_depthStencilDXTDC = addDX12TextureDataComponent();
	l_DXRPC->m_depthStencilDXTDC->m_textureDataDesc = DX12RenderingSystemComponent::get().m_deferredRenderPassDesc.RTDesc;
	l_DXRPC->m_depthStencilDXTDC->m_textureDataDesc.usageType = TextureUsageType::DEPTH_STENCIL_ATTACHMENT;
	l_DXRPC->m_depthStencilDXTDC->m_textureData = { nullptr };

	initializeDX12TextureDataComponent(l_DXRPC->m_depthStencilDXTDC);

	l_result &= createDSVDescriptorHeap(l_DXRPC);
	l_result &= createDSV(l_DXRPC);

	// Setup root signature.
	CD3DX12_DESCRIPTOR_RANGE1 l_bassPassRT0DescRange;
	l_bassPassRT0DescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE1 l_samplerDescRange;
	l_samplerDescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	CD3DX12_ROOT_PARAMETER1 l_rootParams[2];
	l_rootParams[0].InitAsDescriptorTable(1, &l_bassPassRT0DescRange, D3D12_SHADER_VISIBILITY_PIXEL);
	l_rootParams[1].InitAsDescriptorTable(1, &l_samplerDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC l_rootSigDesc(sizeof(l_rootParams) / sizeof(l_rootParams[0]), l_rootParams);
	l_DXRPC->m_rootSignatureDesc = l_rootSigDesc;
	l_DXRPC->m_rootSignatureDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Setup the description of the depth stencil state.
	l_DXRPC->m_depthStencilDesc.DepthEnable = true;
	l_DXRPC->m_depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	l_DXRPC->m_depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	l_DXRPC->m_depthStencilDesc.StencilEnable = true;
	l_DXRPC->m_depthStencilDesc.StencilReadMask = 0xFF;
	l_DXRPC->m_depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	l_DXRPC->m_depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	l_DXRPC->m_depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	l_DXRPC->m_depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	l_DXRPC->m_depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Stencil operations if pixel is back-facing.
	l_DXRPC->m_depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	l_DXRPC->m_depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	l_DXRPC->m_depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	l_DXRPC->m_depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	l_DXRPC->m_rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	l_DXRPC->m_blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// Setup the viewport.
	l_DXRPC->m_viewport.Width = (float)l_DXRPC->m_renderPassDesc.RTDesc.width;
	l_DXRPC->m_viewport.Height = (float)l_DXRPC->m_renderPassDesc.RTDesc.height;
	l_DXRPC->m_viewport.MinDepth = 0.0f;
	l_DXRPC->m_viewport.MaxDepth = 1.0f;
	l_DXRPC->m_viewport.TopLeftX = 0.0f;
	l_DXRPC->m_viewport.TopLeftY = 0.0f;

	// Setup the scissor rect.
	l_DXRPC->m_scissor.left = 0;
	l_DXRPC->m_scissor.top = 0;
	l_DXRPC->m_scissor.right = (unsigned long)l_DXRPC->m_viewport.Width;
	l_DXRPC->m_scissor.bottom = (unsigned long)l_DXRPC->m_viewport.Height;

	// Setup PSO.
	l_DXRPC->m_PSODesc.SampleMask = UINT_MAX;
	l_DXRPC->m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	l_DXRPC->m_PSODesc.SampleDesc.Count = 1;

	l_result = createRootSignature(l_DXRPC);
	l_result = createPSO(l_DXRPC, l_DXSPC);
	l_result = createCommandQueue(l_DXRPC);
	l_result = createCommandAllocators(l_DXRPC);
	l_result = createCommandLists(l_DXRPC);

	DX12RenderingSystemComponent::get().m_swapChainDXRPC = l_DXRPC;
	DX12RenderingSystemComponent::get().m_swapChainDXSPC = l_DXSPC;

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

	l_result = l_result && createGlobalCommandAllocator();

	l_result = l_result && createGlobalCSUHeap();
	l_result = l_result && createGlobalSamplerHeap();

	loadDefaultAssets();

	generateGPUBuffers();

	l_result = l_result && createSwapChain();
	l_result = l_result && createSwapChainDXRPC();

	l_result = l_result && createSwapChainSyncPrimitives();

	DX12OpaquePass::initialize();
	DX12LightPass::initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem has been initialized.");

	return l_result;
}

bool DX12RenderingSystemNS::update()
{
	updateConstantBuffer(DX12RenderingSystemComponent::get().m_cameraConstantBuffer, RenderingFrontendSystemComponent::get().m_cameraGPUData);
	updateConstantBuffer(DX12RenderingSystemComponent::get().m_sunConstantBuffer, RenderingFrontendSystemComponent::get().m_sunGPUData);
	updateConstantBuffer(DX12RenderingSystemComponent::get().m_pointLightConstantBuffer, RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector);
	updateConstantBuffer(DX12RenderingSystemComponent::get().m_sphereLightConstantBuffer, RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector);
	updateConstantBuffer(DX12RenderingSystemComponent::get().m_skyConstantBuffer, RenderingFrontendSystemComponent::get().m_skyGPUData);

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
	DX12LightPass::update();

	auto l_MDC = getDX12MeshDataComponent(MeshShapeType::QUAD);
	auto l_swapChainDXRPC = DX12RenderingSystemComponent::get().m_swapChainDXRPC;
	auto l_frameIndex = l_swapChainDXRPC->m_frameIndex;

	recordCommandBegin(l_swapChainDXRPC, l_frameIndex);

	l_swapChainDXRPC->m_commandLists[l_frameIndex]->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_swapChainDXRPC->m_DXTDCs[l_frameIndex]->m_texture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	recordActivateRenderPass(l_swapChainDXRPC, l_frameIndex);

	ID3D12DescriptorHeap* l_heaps[] = { DX12RenderingSystemComponent::get().m_CSUHeap, DX12RenderingSystemComponent::get().m_samplerHeap };
	recordBindDescHeaps(l_swapChainDXRPC, l_frameIndex, 2, l_heaps);

	recordBindTextureForRead(l_swapChainDXRPC, l_frameIndex, DX12LightPass::getDX12RPC()->m_DXTDCs[0]);

	recordBindSRVDescTable(l_swapChainDXRPC, l_frameIndex, 0, DX12LightPass::getDX12RPC()->m_DXTDCs[0]);
	recordBindSamplerDescTable(l_swapChainDXRPC, l_frameIndex, 1, DX12RenderingSystemComponent::get().m_swapChainDXSPC);

	recordDrawCall(l_swapChainDXRPC, l_frameIndex, l_MDC);

	recordBindTextureForWrite(l_swapChainDXRPC, l_frameIndex, DX12LightPass::getDX12RPC()->m_DXTDCs[0]);

	l_swapChainDXRPC->m_commandLists[l_frameIndex]->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_swapChainDXRPC->m_DXTDCs[l_frameIndex]->m_texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	recordCommandEnd(l_swapChainDXRPC, l_frameIndex);

	return true;
}

bool DX12RenderingSystemNS::render()
{
	DX12OpaquePass::render();
	DX12LightPass::render();

	auto l_swapChainDXRPC = DX12RenderingSystemComponent::get().m_swapChainDXRPC;

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { l_swapChainDXRPC->m_commandLists[l_swapChainDXRPC->m_frameIndex] };
	g_DXRenderingSystemComponent->m_globalCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	DX12RenderingSystemComponent::get().m_swapChain->Present(1, 0);

	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = l_swapChainDXRPC->m_fenceStatus[l_swapChainDXRPC->m_frameIndex];
	g_DXRenderingSystemComponent->m_globalCommandQueue->Signal(l_swapChainDXRPC->m_fence, currentFenceValue);

	// Update the frame index.
	l_swapChainDXRPC->m_frameIndex = DX12RenderingSystemComponent::get().m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (l_swapChainDXRPC->m_fence->GetCompletedValue() < currentFenceValue)
	{
		waitFrame(l_swapChainDXRPC, l_swapChainDXRPC->m_frameIndex);
	}

	// Set the fence value for the next frame.
	l_swapChainDXRPC->m_fenceStatus[l_swapChainDXRPC->m_frameIndex] = currentFenceValue + 1;

	return true;
}

bool DX12RenderingSystemNS::terminate()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->SetFullscreenState(false, NULL);
	}

	// Release the command allocator.
	if (g_DXRenderingSystemComponent->m_globalCommandAllocator)
	{
		g_DXRenderingSystemComponent->m_globalCommandAllocator->Release();
		g_DXRenderingSystemComponent->m_globalCommandAllocator = 0;
	}

	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->Release();
		g_DXRenderingSystemComponent->m_swapChain = 0;
	}

	// Release the command queue.
	if (g_DXRenderingSystemComponent->m_globalCommandQueue)
	{
		g_DXRenderingSystemComponent->m_globalCommandQueue->Release();
		g_DXRenderingSystemComponent->m_globalCommandQueue = 0;
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
	g_DXRenderingSystemComponent->m_sunConstantBuffer = createConstantBuffer(sizeof(SunGPUData), 1, L"sunConstantBuffer");
	g_DXRenderingSystemComponent->m_pointLightConstantBuffer = createConstantBuffer(sizeof(PointLightGPUData), RenderingFrontendSystemComponent::get().m_maxPointLights, L"pointLightConstantBuffer");
	g_DXRenderingSystemComponent->m_sphereLightConstantBuffer = createConstantBuffer(sizeof(SphereLightGPUData), RenderingFrontendSystemComponent::get().m_maxSphereLights, L"sphereLightConstantBuffer");
	g_DXRenderingSystemComponent->m_skyConstantBuffer = createConstantBuffer(sizeof(SkyGPUData), 1, L"skyConstantBuffer");

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
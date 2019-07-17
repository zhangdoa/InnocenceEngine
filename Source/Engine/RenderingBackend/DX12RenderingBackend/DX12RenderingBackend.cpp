#include "DX12RenderingBackend.h"

#include "DX12RenderingBackendUtilities.h"

#include "DX12OpaquePass.h"
#include "DX12LightPass.h"

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
	bool createSwapChainDXRPC();
	bool createSwapChainSyncPrimitives();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_entityID;

	static DX12RenderingBackendComponent* g_DXRenderingBackendComponent;

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

bool DX12RenderingBackendNS::createDebugCallback()
{
	ID3D12Debug* l_debugInterface;

	auto l_result = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't get DirectX 12 debug interface!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	l_result = l_debugInterface->QueryInterface(IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_debugInterface));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't query DirectX 12 debug interface!");
		m_objectStatus = ObjectStatus::Suspended;
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

	// Create a DirectX graphics interface factory.
	l_result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_factory));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create DXGI factory!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: DXGI factory has been created.");

	// Use the factory to create an adapter for the primary graphics interface (video card).
	auto l_adapter1 = getHardwareAdapter(g_DXRenderingBackendComponent->m_factory);

	if (l_adapter1 == nullptr)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create a suitable video card adapter!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_DXRenderingBackendComponent->m_adapter = reinterpret_cast<IDXGIAdapter4*>(l_adapter1);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: Video card adapter has been created.");

	// Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
	// Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
	auto featureLevel = D3D_FEATURE_LEVEL_12_1;

	// Create the Direct3D 12 device.
	l_result = D3D12CreateDevice(g_DXRenderingBackendComponent->m_adapter, featureLevel, IID_PPV_ARGS(&g_DXRenderingBackendComponent->m_device));
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create a DirectX 12.1 device. The default video card does not support DirectX 12.1!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: D3D device has been created.");

	// Set debug report severity
	auto l_pInfoQueue = reinterpret_cast<ID3D12InfoQueue*>(g_DXRenderingBackendComponent->m_device);

	l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	//l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
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
		m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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

bool DX12RenderingBackendNS::createSwapChainDXRPC()
{
	auto l_imageCount = 2;

	auto l_DXSPC = addDX12ShaderProgramComponent(m_entityID);

	ShaderFilePaths m_shaderFilePaths = { "DX12//finalBlendPassVertex.hlsl/", "", "", "", "DX12//finalBlendPassPixel.hlsl/" };

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

	auto l_DXRPC = addDX12RenderPassComponent(m_entityID, "SwapChainDXRPC\\");

	l_DXRPC->m_renderPassDesc = DX12RenderingBackendComponent::get().m_deferredRenderPassDesc;
	l_DXRPC->m_renderPassDesc.RTNumber = l_imageCount;
	l_DXRPC->m_renderPassDesc.useMultipleFramebuffers = true;
	l_DXRPC->m_renderPassDesc.useDepthAttachment = true;
	l_DXRPC->m_renderPassDesc.useStencilAttachment = true;

	// initialize manually
	bool l_result = true;
	l_result &= reserveRenderTargets(l_DXRPC);

	// use local command queue for swap chain
	l_result = createCommandQueue(l_DXRPC);
	l_result = createCommandAllocators(l_DXRPC);
	l_result = createCommandLists(l_DXRPC);

	// create swap chain
	// Set the swap chain to use double buffering.
	g_DXRenderingBackendComponent->m_swapChainDesc.BufferCount = 2;

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
		l_DXRPC->m_commandQueue,
		WinWindowSystemComponent::get().m_hwnd,
		&g_DXRenderingBackendComponent->m_swapChainDesc,
		nullptr,
		nullptr,
		&l_swapChain1);

	g_DXRenderingBackendComponent->m_swapChain = reinterpret_cast<IDXGISwapChain4*>(l_swapChain1);

	if (FAILED(l_hResult))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't create swap chain!");
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend: Swap chain has been created.");

	// use device created swap chain textures
	for (size_t i = 0; i < l_imageCount; i++)
	{
		auto l_result = g_DXRenderingBackendComponent->m_swapChain->GetBuffer((unsigned int)i, IID_PPV_ARGS(&l_DXRPC->m_renderTargets[i]->m_texture));
		if (FAILED(l_result))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: Can't get pointer of swap chain render target " + std::to_string(i) + "!");
			m_objectStatus = ObjectStatus::Suspended;
			return false;
		}
		l_DXRPC->m_renderTargets[i]->m_DX12TextureDataDesc = l_DXRPC->m_renderTargets[i]->m_texture->GetDesc();
	}

	l_DXRPC->m_depthStencilTarget = addDX12TextureDataComponent();
	l_DXRPC->m_depthStencilTarget->m_textureDataDesc = DX12RenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc;
	l_DXRPC->m_depthStencilTarget->m_textureDataDesc.usageType = TextureUsageType::DEPTH_STENCIL_ATTACHMENT;
	l_DXRPC->m_depthStencilTarget->m_textureData = { nullptr };

	initializeDX12TextureDataComponent(l_DXRPC->m_depthStencilTarget);

	l_result &= createRTVDescriptorHeap(l_DXRPC);
	l_result &= createRTV(l_DXRPC);

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

	DX12RenderingBackendComponent::get().m_swapChainDXRPC = l_DXRPC;
	DX12RenderingBackendComponent::get().m_swapChainDXSPC = l_DXSPC;

	return true;
}

bool DX12RenderingBackendNS::createSwapChainSyncPrimitives()
{
	return createSyncPrimitives(DX12RenderingBackendComponent::get().m_swapChainDXRPC);
}

bool DX12RenderingBackendNS::setup()
{
	m_entityID = InnoMath::createEntityID();

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

	m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingBackend setup finished.");

	return l_result;
}

bool DX12RenderingBackendNS::initialize()
{
	if (DX12RenderingBackendNS::m_objectStatus == ObjectStatus::Created)
	{
		auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

		m_MeshDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(DX12MeshDataComponent), l_renderingCapability.maxMeshes);
		m_MaterialDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(MaterialDataComponent), l_renderingCapability.maxMaterials);
		m_TextureDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(DX12TextureDataComponent), l_renderingCapability.maxTextures);

		bool l_result = true;

		l_result &= createGlobalCommandQueue();
		l_result &= createGlobalCommandAllocator();

		l_result &= createGlobalCSUHeap();
		l_result &= createGlobalSamplerHeap();

		loadDefaultAssets();

		generateGPUBuffers();

		l_result &= createSwapChainDXRPC();
		l_result &= createSwapChainSyncPrimitives();

		DX12OpaquePass::initialize();
		DX12LightPass::initialize();

		DX12RenderingBackendNS::m_objectStatus = ObjectStatus::Activated;
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
	while (DX12RenderingBackendNS::m_uninitializedMDC.size() > 0)
	{
		DX12MeshDataComponent* l_MDC;
		DX12RenderingBackendNS::m_uninitializedMDC.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeDX12MeshDataComponent(l_MDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't create DX12MeshDataComponent for " + std::string(l_MDC->m_parentEntity->m_entityName.c_str()) + "!");
			}
		}
	}
	while (DX12RenderingBackendNS::m_uninitializedTDC.size() > 0)
	{
		DX12TextureDataComponent* l_TDC;
		DX12RenderingBackendNS::m_uninitializedTDC.tryPop(l_TDC);

		if (l_TDC)
		{
			auto l_result = initializeDX12TextureDataComponent(l_TDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingBackend: can't create DX12TextureDataComponent for " + std::string(l_TDC->m_parentEntity->m_entityName.c_str()) + "!");
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

	auto l_MDC = getDX12MeshDataComponent(MeshShapeType::QUAD);
	auto l_swapChainDXRPC = DX12RenderingBackendComponent::get().m_swapChainDXRPC;
	auto l_frameIndex = l_swapChainDXRPC->m_currentFrameIndex;

	recordCommandBegin(l_swapChainDXRPC, l_frameIndex);

	l_swapChainDXRPC->m_commandLists[l_frameIndex]->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_swapChainDXRPC->m_renderTargets[l_frameIndex]->m_texture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	recordActivateRenderPass(l_swapChainDXRPC, l_frameIndex);

	ID3D12DescriptorHeap* l_heaps[] = { DX12RenderingBackendComponent::get().m_CSUHeap, DX12RenderingBackendComponent::get().m_samplerHeap };
	recordBindDescHeaps(l_swapChainDXRPC, l_frameIndex, 2, l_heaps);

	recordBindRTForRead(l_swapChainDXRPC, l_frameIndex, DX12LightPass::getDX12RPC()->m_renderTargets[0]);

	recordBindSRVDescTable(l_swapChainDXRPC, l_frameIndex, 0, DX12LightPass::getDX12RPC()->m_SRVs[0]);
	recordBindSamplerDescTable(l_swapChainDXRPC, l_frameIndex, 1, DX12RenderingBackendComponent::get().m_swapChainDXSPC);

	recordDrawCall(l_swapChainDXRPC, l_frameIndex, l_MDC);

	recordBindRTForWrite(l_swapChainDXRPC, l_frameIndex, DX12LightPass::getDX12RPC()->m_renderTargets[0]);

	l_swapChainDXRPC->m_commandLists[l_frameIndex]->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(l_swapChainDXRPC->m_renderTargets[l_frameIndex]->m_texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	recordCommandEnd(l_swapChainDXRPC, l_frameIndex);

	return true;
}

bool DX12RenderingBackendNS::render()
{
	DX12OpaquePass::render();
	waitFrame(DX12OpaquePass::getDX12RPC(), 0);

	DX12LightPass::render();
	waitFrame(DX12LightPass::getDX12RPC(), 0);

	auto l_swapChainDXRPC = DX12RenderingBackendComponent::get().m_swapChainDXRPC;

	auto l_currentFrameIndex = l_swapChainDXRPC->m_currentFrameIndex;

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { l_swapChainDXRPC->m_commandLists[l_currentFrameIndex] };
	l_swapChainDXRPC->m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	DX12RenderingBackendComponent::get().m_swapChain->Present(1, 0);

	// Schedule a Signal command in the queue.
	const UINT64 l_currentFenceValue = l_swapChainDXRPC->m_fenceStatus[l_currentFrameIndex];
	l_swapChainDXRPC->m_commandQueue->Signal(l_swapChainDXRPC->m_fence, l_currentFenceValue);

	auto l_nextFrameIndex = DX12RenderingBackendComponent::get().m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (l_swapChainDXRPC->m_fence->GetCompletedValue() < l_currentFenceValue)
	{
		waitFrame(l_swapChainDXRPC, l_nextFrameIndex);
	}

	// Set the fence value for the next frame.
	l_swapChainDXRPC->m_fenceStatus[l_nextFrameIndex] = l_currentFenceValue + 1;

	// Update the frame index.
	l_swapChainDXRPC->m_currentFrameIndex = l_nextFrameIndex;

	return true;
}

bool DX12RenderingBackendNS::terminate()
{
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

	destroyDX12RenderPassComponent(DX12RenderingBackendComponent::get().m_swapChainDXRPC);

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

	m_objectStatus = ObjectStatus::Terminated;
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
	g_pModuleManager->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMDC);
	m_unitQuadMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitQuadMDC->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMDC);

	m_unitCubeMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addDX12MeshDataComponent();
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

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
	static std::atomic<unsigned int> meshCount = 0;
	meshCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(DX12MeshDataComponent));
	auto l_MDC = new(l_rawPtr)DX12MeshDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Mesh_" + std::to_string(meshCount) + "/").c_str());
	l_MDC->m_parentEntity = l_parentEntity;
	return l_MDC;
}

MaterialDataComponent* DX12RenderingBackendNS::addMaterialDataComponent()
{
	static std::atomic<unsigned int> materialCount = 0;
	materialCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(MaterialDataComponent));
	auto l_MDC = new(l_rawPtr)MaterialDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Material_" + std::to_string(materialCount) + "/").c_str());
	l_MDC->m_parentEntity = l_parentEntity;
	return l_MDC;
}

DX12TextureDataComponent* DX12RenderingBackendNS::addDX12TextureDataComponent()
{
	static std::atomic<unsigned int> textureCount = 0;
	textureCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(DX12TextureDataComponent));
	auto l_TDC = new(l_rawPtr)DX12TextureDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Texture_" + std::to_string(textureCount) + "/").c_str());
	l_TDC->m_parentEntity = l_parentEntity;
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
		return DX12RenderingBackendNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return DX12RenderingBackendNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return DX12RenderingBackendNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return DX12RenderingBackendNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return DX12RenderingBackendNS::m_basicAOTDC; break;
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

bool DX12RenderingBackend::terminate()
{
	return DX12RenderingBackendNS::terminate();
}

ObjectStatus DX12RenderingBackend::getStatus()
{
	return DX12RenderingBackendNS::m_objectStatus;
}

MeshDataComponent * DX12RenderingBackend::addMeshDataComponent()
{
	return DX12RenderingBackendNS::addDX12MeshDataComponent();
}

MaterialDataComponent * DX12RenderingBackend::addMaterialDataComponent()
{
	return DX12RenderingBackendNS::addMaterialDataComponent();
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
	DX12RenderingBackendNS::m_uninitializedMDC.push(reinterpret_cast<DX12MeshDataComponent*>(rhs));
}

void DX12RenderingBackend::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	DX12RenderingBackendNS::m_uninitializedTDC.push(reinterpret_cast<DX12TextureDataComponent*>(rhs));
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
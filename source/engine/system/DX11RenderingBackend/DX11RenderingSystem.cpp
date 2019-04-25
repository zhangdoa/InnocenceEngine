#include "DX11RenderingSystem.h"

#include "DX11RenderingSystemUtilities.h"

#include "DX11OpaquePass.h"
#include "DX11LightCullingPass.h"
#include "DX11LightPass.h"
#include "DX11SkyPass.h"
#include "DX11PreTAAPass.h"
#include "DX11TAAPass.h"
#include "DX11FinalBlendPass.h"

#include "../../component/DX11RenderingSystemComponent.h"
#include "../../component/WinWindowSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DX11RenderingSystemNS
{
	bool createPhysicalDevices();
	bool createSwapChain();
	bool createBackBuffer();
	bool createRasterizer();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	static DX11RenderingSystemComponent* g_DXRenderingSystemComponent;

	ThreadSafeUnorderedMap<EntityID, DX11MeshDataComponent*> m_meshMap;
	ThreadSafeUnorderedMap<EntityID, MaterialDataComponent*> m_materialMap;
	ThreadSafeUnorderedMap<EntityID, DX11TextureDataComponent*> m_textureMap;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<DX11MeshDataComponent*> m_uninitializedMDC;
	ThreadSafeQueue<DX11TextureDataComponent*> m_uninitializedTDC;

	DX11TextureDataComponent* m_iconTemplate_OBJ;
	DX11TextureDataComponent* m_iconTemplate_PNG;
	DX11TextureDataComponent* m_iconTemplate_SHADER;
	DX11TextureDataComponent* m_iconTemplate_UNKNOWN;

	DX11TextureDataComponent* m_iconTemplate_DirectionalLight;
	DX11TextureDataComponent* m_iconTemplate_PointLight;
	DX11TextureDataComponent* m_iconTemplate_SphereLight;

	DX11MeshDataComponent* m_unitLineMDC;
	DX11MeshDataComponent* m_unitQuadMDC;
	DX11MeshDataComponent* m_unitCubeMDC;
	DX11MeshDataComponent* m_unitSphereMDC;
	DX11MeshDataComponent* m_terrainMDC;

	DX11TextureDataComponent* m_basicNormalTDC;
	DX11TextureDataComponent* m_basicAlbedoTDC;
	DX11TextureDataComponent* m_basicMetallicTDC;
	DX11TextureDataComponent* m_basicRoughnessTDC;
	DX11TextureDataComponent* m_basicAOTDC;
}

bool DX11RenderingSystemNS::createPhysicalDevices()
{
	HRESULT result;

	unsigned int numModes;
	unsigned long long stringLength;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&g_DXRenderingSystemComponent->m_factory);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create DXGI factory!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = g_DXRenderingSystemComponent->m_factory->EnumAdapters(0, &g_DXRenderingSystemComponent->m_adapter);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create video card adapter!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = g_DXRenderingSystemComponent->m_adapter->EnumOutputs(0, &g_DXRenderingSystemComponent->m_adapterOutput);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create monitor adapter!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = g_DXRenderingSystemComponent->m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	std::vector<DXGI_MODE_DESC> displayModeList(numModes);

	// Now fill the display mode list structures.
	result = g_DXRenderingSystemComponent->m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, &displayModeList[0]);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't fill the display mode list structures!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	for (unsigned int i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == l_screenResolution.x
			&&
			displayModeList[i].Height == l_screenResolution.y
			)
		{
			g_DXRenderingSystemComponent->m_refreshRate.x = displayModeList[i].RefreshRate.Numerator;
			g_DXRenderingSystemComponent->m_refreshRate.y = displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Get the adapter (video card) description.
	result = g_DXRenderingSystemComponent->m_adapter->GetDesc(&g_DXRenderingSystemComponent->m_adapterDesc);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't get the video card adapter description!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	g_DXRenderingSystemComponent->m_videoCardMemory = (int)(g_DXRenderingSystemComponent->m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	if (wcstombs_s(&stringLength, g_DXRenderingSystemComponent->m_videoCardDescription, 128, g_DXRenderingSystemComponent->m_adapterDesc.Description, 128) != 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't convert the name of the video card to a character array!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Release the display mode list.
	// displayModeList.clear();

	// Release the adapter output.
	g_DXRenderingSystemComponent->m_adapterOutput->Release();
	g_DXRenderingSystemComponent->m_adapterOutput = 0;

	// Release the adapter.
	g_DXRenderingSystemComponent->m_adapter->Release();
	g_DXRenderingSystemComponent->m_adapter = 0;

	// Release the factory.
	g_DXRenderingSystemComponent->m_factory->Release();
	g_DXRenderingSystemComponent->m_factory = 0;

	return true;
}

bool DX11RenderingSystemNS::createSwapChain()
{
	HRESULT result;
	D3D_FEATURE_LEVEL featureLevel;

	// Initialize the swap chain description.
	ZeroMemory(&g_DXRenderingSystemComponent->m_swapChainDesc, sizeof(g_DXRenderingSystemComponent->m_swapChainDesc));

	// Set to a single back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferCount = 1;

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	// Set the width and height of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Width = l_screenResolution.x;
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Height = l_screenResolution.y;

	// Set regular 32-bit surface for the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (WinWindowSystemComponent::get().m_vsync_enabled)
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Numerator = g_DXRenderingSystemComponent->m_refreshRate.x;
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Denominator = g_DXRenderingSystemComponent->m_refreshRate.y;
	}
	else
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	g_DXRenderingSystemComponent->m_swapChainDesc.OutputWindow = WinWindowSystemComponent::get().m_hwnd;

	// Turn multisampling off.
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Count = 1;
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	// @TODO: finish this feature
	g_DXRenderingSystemComponent->m_swapChainDesc.Windowed = true;

	// Set the scan line ordering and scaling to unspecified.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	g_DXRenderingSystemComponent->m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	g_DXRenderingSystemComponent->m_swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	unsigned int creationFlags = 0;
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, &featureLevel, 1,
		D3D11_SDK_VERSION, &g_DXRenderingSystemComponent->m_swapChainDesc, &g_DXRenderingSystemComponent->m_swapChain, &g_DXRenderingSystemComponent->m_device, NULL, &g_DXRenderingSystemComponent->m_deviceContext);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the swap chain/D3D device/D3D device context!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

bool DX11RenderingSystemNS::createBackBuffer()
{
	HRESULT result;

	// Get the pointer to the back buffer.
	result = g_DXRenderingSystemComponent->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&g_DXRenderingSystemComponent->m_renderTargetTexture);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't get back buffer pointer!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = g_DXRenderingSystemComponent->m_device->CreateRenderTargetView(g_DXRenderingSystemComponent->m_renderTargetTexture, NULL, &g_DXRenderingSystemComponent->m_renderTargetView);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create render target view!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	g_DXRenderingSystemComponent->m_renderTargetTexture->Release();
	g_DXRenderingSystemComponent->m_renderTargetTexture = 0;

	// Initialize the description of the depth buffer.
	ZeroMemory(&g_DXRenderingSystemComponent->m_depthTextureDesc, sizeof(g_DXRenderingSystemComponent->m_depthTextureDesc));

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	// Set up the description of the depth buffer.
	g_DXRenderingSystemComponent->m_depthTextureDesc.Width = l_screenResolution.x;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Height = l_screenResolution.y;
	g_DXRenderingSystemComponent->m_depthTextureDesc.MipLevels = 1;
	g_DXRenderingSystemComponent->m_depthTextureDesc.ArraySize = 1;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	g_DXRenderingSystemComponent->m_depthTextureDesc.SampleDesc.Count = 1;
	g_DXRenderingSystemComponent->m_depthTextureDesc.SampleDesc.Quality = 0;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	g_DXRenderingSystemComponent->m_depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	g_DXRenderingSystemComponent->m_depthTextureDesc.CPUAccessFlags = 0;
	g_DXRenderingSystemComponent->m_depthTextureDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = g_DXRenderingSystemComponent->m_device->CreateTexture2D(&g_DXRenderingSystemComponent->m_depthTextureDesc, NULL, &g_DXRenderingSystemComponent->m_depthStencilTexture);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the texture for the depth buffer!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Initialize the description of the stencil state.
	ZeroMemory(&g_DXRenderingSystemComponent->m_depthStencilDesc, sizeof(g_DXRenderingSystemComponent->m_depthStencilDesc));

	// Set up the description of the stencil state.
	g_DXRenderingSystemComponent->m_depthStencilDesc.DepthEnable = true;
	g_DXRenderingSystemComponent->m_depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	g_DXRenderingSystemComponent->m_depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	g_DXRenderingSystemComponent->m_depthStencilDesc.StencilEnable = true;
	g_DXRenderingSystemComponent->m_depthStencilDesc.StencilReadMask = 0xFF;
	g_DXRenderingSystemComponent->m_depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = g_DXRenderingSystemComponent->m_device->CreateDepthStencilState(
		&g_DXRenderingSystemComponent->m_depthStencilDesc,
		&g_DXRenderingSystemComponent->m_defaultDepthStencilState);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the depth stencil state!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Initialize the depth stencil view.
	ZeroMemory(&g_DXRenderingSystemComponent->m_depthStencilViewDesc, sizeof(
		g_DXRenderingSystemComponent->m_depthStencilViewDesc));

	// Set up the depth stencil view description.
	g_DXRenderingSystemComponent->m_depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	g_DXRenderingSystemComponent->m_depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	g_DXRenderingSystemComponent->m_depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = g_DXRenderingSystemComponent->m_device->CreateDepthStencilView(
		g_DXRenderingSystemComponent->m_depthStencilTexture,
		&g_DXRenderingSystemComponent->m_depthStencilViewDesc,
		&g_DXRenderingSystemComponent->m_depthStencilView);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the depth stencil view!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	g_DXRenderingSystemComponent->m_deviceContext->OMSetRenderTargets(
		1,
		&g_DXRenderingSystemComponent->m_renderTargetView,
		g_DXRenderingSystemComponent->m_depthStencilView);

	return true;
}

bool DX11RenderingSystemNS::createRasterizer()
{
	HRESULT result;

	// Setup the raster description which will determine how and what polygons will be drawn.
	g_DXRenderingSystemComponent->m_rasterDescForward.AntialiasedLineEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescForward.CullMode = D3D11_CULL_NONE;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthBias = 0;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthBiasClamp = 0.0f;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthClipEnable = true;
	g_DXRenderingSystemComponent->m_rasterDescForward.FillMode = D3D11_FILL_SOLID;
	g_DXRenderingSystemComponent->m_rasterDescForward.FrontCounterClockwise = true;
	g_DXRenderingSystemComponent->m_rasterDescForward.MultisampleEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescForward.ScissorEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescForward.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state for forward pass
	result = g_DXRenderingSystemComponent->m_device->CreateRasterizerState(
		&g_DXRenderingSystemComponent->m_rasterDescForward,
		&g_DXRenderingSystemComponent->m_rasterStateForward);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the rasterizer state for forward pass!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_rasterDescDeferred.AntialiasedLineEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.CullMode = D3D11_CULL_NONE;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthBias = 0;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthBiasClamp = 0.0f;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthClipEnable = true;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.FillMode = D3D11_FILL_SOLID;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.FrontCounterClockwise = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.MultisampleEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.ScissorEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state for deferred pass
	result = g_DXRenderingSystemComponent->m_device->CreateRasterizerState(
		&g_DXRenderingSystemComponent->m_rasterDescDeferred,
		&g_DXRenderingSystemComponent->m_rasterStateDeferred);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the rasterizer state for deferred pass!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Setup the viewport for rendering.
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	g_DXRenderingSystemComponent->m_viewport.Width = (float)l_screenResolution.x;
	g_DXRenderingSystemComponent->m_viewport.Height = (float)l_screenResolution.y;
	g_DXRenderingSystemComponent->m_viewport.MinDepth = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.MaxDepth = 1.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftX = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftY = 0.0f;

	return true;
}

bool DX11RenderingSystemNS::setup()
{
	initializeComponentPool();

	g_DXRenderingSystemComponent = &DX11RenderingSystemComponent::get();

	bool result = true;
	result = result && initializeComponentPool();
	result = result && createPhysicalDevices();
	result = result && createSwapChain();
	result = result && createBackBuffer();
	result = result && createRasterizer();

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	// Setup the description of the deferred pass.
	g_DXRenderingSystemComponent->deferredPassTextureDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.width = l_screenResolution.x;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.height = l_screenResolution.y;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT16;

	g_DXRenderingSystemComponent->deferredPassRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	g_DXRenderingSystemComponent->deferredPassRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	g_DXRenderingSystemComponent->deferredPassRTVDesc.Texture2D.MipSlice = 0;

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingSystem setup finished.");
	return result;
}

bool DX11RenderingSystemNS::initialize()
{
	m_MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX11MeshDataComponent), 16384);
	m_MaterialDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MaterialDataComponent), 32768);
	m_TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX11TextureDataComponent), 32768);

	loadDefaultAssets();

	generateGPUBuffers();

	DX11OpaquePass::initialize();
	DX11LightCullingPass::initialize();
	DX11LightPass::initialize();
	DX11SkyPass::initialize();
	DX11PreTAAPass::initialize();
	DX11TAAPass::initialize();
	DX11FinalBlendPass::initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingSystem has been initialized.");

	return true;
}

void DX11RenderingSystemNS::loadDefaultAssets()
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

	m_basicNormalTDC = reinterpret_cast<DX11TextureDataComponent*>(l_basicNormalTDC);
	m_basicAlbedoTDC = reinterpret_cast<DX11TextureDataComponent*>(l_basicAlbedoTDC);
	m_basicMetallicTDC = reinterpret_cast<DX11TextureDataComponent*>(l_basicMetallicTDC);
	m_basicRoughnessTDC = reinterpret_cast<DX11TextureDataComponent*>(l_basicRoughnessTDC);
	m_basicAOTDC = reinterpret_cast<DX11TextureDataComponent*>(l_basicAOTDC);

	m_iconTemplate_OBJ = reinterpret_cast<DX11TextureDataComponent*>(l_iconTemplate_OBJ);
	m_iconTemplate_PNG = reinterpret_cast<DX11TextureDataComponent*>(l_iconTemplate_PNG);
	m_iconTemplate_SHADER = reinterpret_cast<DX11TextureDataComponent*>(l_iconTemplate_SHADER);
	m_iconTemplate_UNKNOWN = reinterpret_cast<DX11TextureDataComponent*>(l_iconTemplate_UNKNOWN);

	m_iconTemplate_DirectionalLight = reinterpret_cast<DX11TextureDataComponent*>(l_iconTemplate_DirectionalLight);
	m_iconTemplate_PointLight = reinterpret_cast<DX11TextureDataComponent*>(l_iconTemplate_PointLight);
	m_iconTemplate_SphereLight = reinterpret_cast<DX11TextureDataComponent*>(l_iconTemplate_SphereLight);

	m_unitLineMDC = addDX11MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addDX11MeshDataComponent();
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

	m_unitCubeMDC = addDX11MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addDX11MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addDX11MeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

	initializeDX11MeshDataComponent(m_unitLineMDC);
	initializeDX11MeshDataComponent(m_unitQuadMDC);
	initializeDX11MeshDataComponent(m_unitCubeMDC);
	initializeDX11MeshDataComponent(m_unitSphereMDC);
	initializeDX11MeshDataComponent(m_terrainMDC);

	initializeDX11TextureDataComponent(m_basicNormalTDC);
	initializeDX11TextureDataComponent(m_basicAlbedoTDC);
	initializeDX11TextureDataComponent(m_basicMetallicTDC);
	initializeDX11TextureDataComponent(m_basicRoughnessTDC);
	initializeDX11TextureDataComponent(m_basicAOTDC);

	initializeDX11TextureDataComponent(m_iconTemplate_OBJ);
	initializeDX11TextureDataComponent(m_iconTemplate_PNG);
	initializeDX11TextureDataComponent(m_iconTemplate_SHADER);
	initializeDX11TextureDataComponent(m_iconTemplate_UNKNOWN);

	initializeDX11TextureDataComponent(m_iconTemplate_DirectionalLight);
	initializeDX11TextureDataComponent(m_iconTemplate_PointLight);
	initializeDX11TextureDataComponent(m_iconTemplate_SphereLight);
}

bool DX11RenderingSystemNS::generateGPUBuffers()
{
	g_DXRenderingSystemComponent->m_cameraConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_cameraConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(CameraGPUData);
	g_DXRenderingSystemComponent->m_cameraConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_cameraConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_cameraConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_cameraConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_cameraConstantBuffer);

	g_DXRenderingSystemComponent->m_meshConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_meshConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(MeshGPUData);
	g_DXRenderingSystemComponent->m_meshConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_meshConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_meshConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_meshConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_meshConstantBuffer);

	g_DXRenderingSystemComponent->m_materialConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_materialConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(MaterialGPUData);
	g_DXRenderingSystemComponent->m_materialConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_materialConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_materialConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_materialConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_materialConstantBuffer);

	g_DXRenderingSystemComponent->m_sunConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_sunConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(SunGPUData);
	g_DXRenderingSystemComponent->m_sunConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_sunConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_sunConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_sunConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_sunConstantBuffer);

	g_DXRenderingSystemComponent->m_pointLightConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_pointLightConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(PointLightGPUData) * RenderingFrontendSystemComponent::get().m_maxPointLights;
	g_DXRenderingSystemComponent->m_pointLightConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_pointLightConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_pointLightConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_pointLightConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_pointLightConstantBuffer);

	g_DXRenderingSystemComponent->m_sphereLightConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_sphereLightConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(SphereLightGPUData)* RenderingFrontendSystemComponent::get().m_maxSphereLights;
	g_DXRenderingSystemComponent->m_sphereLightConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_sphereLightConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_sphereLightConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_sphereLightConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_sphereLightConstantBuffer);

	g_DXRenderingSystemComponent->m_skyConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_skyConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(SkyConstantBufferData);
	g_DXRenderingSystemComponent->m_skyConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_skyConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_skyConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_skyConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_skyConstantBuffer);

	g_DXRenderingSystemComponent->m_dispatchParamsConstantBuffer.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	g_DXRenderingSystemComponent->m_dispatchParamsConstantBuffer.m_ConstantBufferDesc.ByteWidth = sizeof(DispatchParamsConstantBufferData);
	g_DXRenderingSystemComponent->m_dispatchParamsConstantBuffer.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_DXRenderingSystemComponent->m_dispatchParamsConstantBuffer.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_DXRenderingSystemComponent->m_dispatchParamsConstantBuffer.m_ConstantBufferDesc.MiscFlags = 0;
	g_DXRenderingSystemComponent->m_dispatchParamsConstantBuffer.m_ConstantBufferDesc.StructureByteStride = 0;
	createConstantBuffer(g_DXRenderingSystemComponent->m_dispatchParamsConstantBuffer);

	return true;
}

bool DX11RenderingSystemNS::update()
{
	if (DX11RenderingSystemNS::m_uninitializedMDC.size() > 0)
	{
		DX11MeshDataComponent* l_MDC;
		DX11RenderingSystemNS::m_uninitializedMDC.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeDX11MeshDataComponent(l_MDC);
			if (!l_result)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create DX11MeshDataComponent for " + l_MDC->m_parentEntity + "!");
			}
		}
	}
	if (DX11RenderingSystemNS::m_uninitializedTDC.size() > 0)
	{
		DX11TextureDataComponent* l_TDC;
		DX11RenderingSystemNS::m_uninitializedTDC.tryPop(l_TDC);

		if (l_TDC)
		{
			auto l_result = initializeDX11TextureDataComponent(l_TDC);
			if (!l_result)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create DX11TextureDataComponent for " + l_TDC->m_parentEntity + "!");
			}
		}
	}

	auto l_viewportSize = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	DX11RenderingSystemComponent::get().m_skyConstantBufferData.viewportSize.x = (float)l_viewportSize.x;
	DX11RenderingSystemComponent::get().m_skyConstantBufferData.viewportSize.y = (float)l_viewportSize.y;
	DX11RenderingSystemComponent::get().m_skyConstantBufferData.p_inv = RenderingFrontendSystemComponent::get().m_cameraGPUData.p_original.inverse();
	DX11RenderingSystemComponent::get().m_skyConstantBufferData.r_inv = RenderingFrontendSystemComponent::get().m_cameraGPUData.r.inverse();

	return true;
}

bool DX11RenderingSystemNS::render()
{
	updateConstantBuffer(DX11RenderingSystemComponent::get().m_cameraConstantBuffer, &RenderingFrontendSystemComponent::get().m_cameraGPUData);
	updateConstantBuffer(DX11RenderingSystemComponent::get().m_sunConstantBuffer, &RenderingFrontendSystemComponent::get().m_sunGPUData);
	updateConstantBuffer(DX11RenderingSystemComponent::get().m_pointLightConstantBuffer, &RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector[0]);
	updateConstantBuffer(DX11RenderingSystemComponent::get().m_sphereLightConstantBuffer, &RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector[0]);
	updateConstantBuffer(DX11RenderingSystemComponent::get().m_skyConstantBuffer, &DX11RenderingSystemComponent::get().m_skyConstantBufferData);

	DX11OpaquePass::update();

	DX11LightCullingPass::update();

	DX11LightPass::update();

	DX11SkyPass::update();

	DX11PreTAAPass::update();

	DX11TAAPass::update();

	DX11FinalBlendPass::update();

	return true;
}

bool DX11RenderingSystemNS::terminate()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->SetFullscreenState(false, NULL);
	}

	if (g_DXRenderingSystemComponent->m_rasterStateDeferred)
	{
		g_DXRenderingSystemComponent->m_rasterStateDeferred->Release();
		g_DXRenderingSystemComponent->m_rasterStateDeferred = 0;
	}

	if (g_DXRenderingSystemComponent->m_depthStencilView)
	{
		g_DXRenderingSystemComponent->m_depthStencilView->Release();
		g_DXRenderingSystemComponent->m_depthStencilView = 0;
	}

	if (g_DXRenderingSystemComponent->m_defaultDepthStencilState)
	{
		g_DXRenderingSystemComponent->m_defaultDepthStencilState->Release();
		g_DXRenderingSystemComponent->m_defaultDepthStencilState = 0;
	}

	if (g_DXRenderingSystemComponent->m_depthStencilTexture)
	{
		g_DXRenderingSystemComponent->m_depthStencilTexture->Release();
		g_DXRenderingSystemComponent->m_depthStencilTexture = 0;
	}

	if (g_DXRenderingSystemComponent->m_renderTargetView)
	{
		g_DXRenderingSystemComponent->m_renderTargetView->Release();
		g_DXRenderingSystemComponent->m_renderTargetView = 0;
	}

	if (g_DXRenderingSystemComponent->m_deviceContext)
	{
		g_DXRenderingSystemComponent->m_deviceContext->Release();
		g_DXRenderingSystemComponent->m_deviceContext = 0;
	}

	if (g_DXRenderingSystemComponent->m_device)
	{
		g_DXRenderingSystemComponent->m_device->Release();
		g_DXRenderingSystemComponent->m_device = 0;
	}

	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->Release();
		g_DXRenderingSystemComponent->m_swapChain = 0;
	}

	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingSystem has been terminated.");
	return true;
}

DX11MeshDataComponent* DX11RenderingSystemNS::addDX11MeshDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(DX11MeshDataComponent));
	auto l_MDC = new(l_rawPtr)DX11MeshDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, DX11MeshDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

MaterialDataComponent* DX11RenderingSystemNS::addMaterialDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(MaterialDataComponent));
	auto l_MDC = new(l_rawPtr)MaterialDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_materialMap = &m_materialMap;
	l_materialMap->emplace(std::pair<EntityID, MaterialDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

DX11TextureDataComponent* DX11RenderingSystemNS::addDX11TextureDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(DX11TextureDataComponent));
	auto l_TDC = new(l_rawPtr)DX11TextureDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_TDC->m_parentEntity = l_parentEntity;
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, DX11TextureDataComponent*>(l_parentEntity, l_TDC));
	return l_TDC;
}

DX11MeshDataComponent* DX11RenderingSystemNS::getDX11MeshDataComponent(EntityID EntityID)
{
	auto result = DX11RenderingSystemNS::m_meshMap.find(EntityID);
	if (result != DX11RenderingSystemNS::m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't find MeshDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

DX11TextureDataComponent * DX11RenderingSystemNS::getDX11TextureDataComponent(EntityID EntityID)
{
	auto result = DX11RenderingSystemNS::m_textureMap.find(EntityID);
	if (result != DX11RenderingSystemNS::m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't find TextureDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

DX11MeshDataComponent* DX11RenderingSystemNS::getDX11MeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return DX11RenderingSystemNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return DX11RenderingSystemNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return DX11RenderingSystemNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return DX11RenderingSystemNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return DX11RenderingSystemNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: wrong MeshShapeType passed to DX11RenderingSystem::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

DX11TextureDataComponent * DX11RenderingSystemNS::getDX11TextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return DX11RenderingSystemNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return DX11RenderingSystemNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return DX11RenderingSystemNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return DX11RenderingSystemNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return DX11RenderingSystemNS::m_basicAOTDC; break;
	case TextureUsageType::COLOR_ATTACHMENT:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

DX11TextureDataComponent * DX11RenderingSystemNS::getDX11TextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return DX11RenderingSystemNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return DX11RenderingSystemNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return DX11RenderingSystemNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return DX11RenderingSystemNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

DX11TextureDataComponent * DX11RenderingSystemNS::getDX11TextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return DX11RenderingSystemNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return DX11RenderingSystemNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return DX11RenderingSystemNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool DX11RenderingSystemNS::resize()
{
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Width = l_screenResolution.x;
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Height = l_screenResolution.y;

	g_DXRenderingSystemComponent->m_depthTextureDesc.Width = l_screenResolution.x;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Height = l_screenResolution.y;

	g_DXRenderingSystemComponent->deferredPassTextureDesc.width = l_screenResolution.x;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.height = l_screenResolution.y;

	g_DXRenderingSystemComponent->m_viewport.Width = (float)l_screenResolution.x;
	g_DXRenderingSystemComponent->m_viewport.Height = (float)l_screenResolution.y;

	DX11OpaquePass::resize();
	DX11LightCullingPass::resize();
	DX11LightPass::resize();
	DX11SkyPass::resize();
	DX11PreTAAPass::resize();
	DX11TAAPass::resize();
	DX11FinalBlendPass::resize();

	return true;
}

bool DX11RenderingSystem::setup()
{
	return DX11RenderingSystemNS::setup();
}

bool DX11RenderingSystem::initialize()
{
	return DX11RenderingSystemNS::initialize();
}

bool DX11RenderingSystem::update()
{
	return DX11RenderingSystemNS::update();
}

bool DX11RenderingSystem::render()
{
	return DX11RenderingSystemNS::render();
}

bool DX11RenderingSystem::terminate()
{
	return DX11RenderingSystemNS::terminate();
}

ObjectStatus DX11RenderingSystem::getStatus()
{
	return DX11RenderingSystemNS::m_objectStatus;
}

MeshDataComponent * DX11RenderingSystem::addMeshDataComponent()
{
	return DX11RenderingSystemNS::addDX11MeshDataComponent();
}

MaterialDataComponent * DX11RenderingSystem::addMaterialDataComponent()
{
	return DX11RenderingSystemNS::addMaterialDataComponent();
}

TextureDataComponent * DX11RenderingSystem::addTextureDataComponent()
{
	return DX11RenderingSystemNS::addDX11TextureDataComponent();
}

MeshDataComponent * DX11RenderingSystem::getMeshDataComponent(EntityID meshID)
{
	return DX11RenderingSystemNS::getDX11MeshDataComponent(meshID);
}

TextureDataComponent * DX11RenderingSystem::getTextureDataComponent(EntityID textureID)
{
	return DX11RenderingSystemNS::getDX11TextureDataComponent(textureID);
}

MeshDataComponent * DX11RenderingSystem::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return DX11RenderingSystemNS::getDX11MeshDataComponent(MeshShapeType);
}

TextureDataComponent * DX11RenderingSystem::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return DX11RenderingSystemNS::getDX11TextureDataComponent(TextureUsageType);
}

TextureDataComponent * DX11RenderingSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	return DX11RenderingSystemNS::getDX11TextureDataComponent(iconType);
}

TextureDataComponent * DX11RenderingSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	return DX11RenderingSystemNS::getDX11TextureDataComponent(iconType);
}

bool DX11RenderingSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &DX11RenderingSystemNS::m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		g_pCoreSystem->getMemorySystem()->destroyObject(DX11RenderingSystemNS::m_MeshDataComponentPool, sizeof(DX11MeshDataComponent), l_mesh->second);
		l_meshMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't remove MeshDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool DX11RenderingSystem::removeTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &DX11RenderingSystemNS::m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO
		}

		g_pCoreSystem->getMemorySystem()->destroyObject(DX11RenderingSystemNS::m_TextureDataComponentPool, sizeof(DX11TextureDataComponent), l_texture->second);
		l_textureMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't remove TextureDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

void DX11RenderingSystem::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	DX11RenderingSystemNS::m_uninitializedMDC.push(reinterpret_cast<DX11MeshDataComponent*>(rhs));
}

void DX11RenderingSystem::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	DX11RenderingSystemNS::m_uninitializedTDC.push(reinterpret_cast<DX11TextureDataComponent*>(rhs));
}

bool DX11RenderingSystem::resize()
{
	return DX11RenderingSystemNS::resize();
}

bool DX11RenderingSystem::reloadShader(RenderPassType renderPassType)
{
	switch (renderPassType)
	{
	case RenderPassType::Shadow:
		break;
	case RenderPassType::Opaque:
		DX11OpaquePass::reloadShaders();
		break;
	case RenderPassType::Light:
		DX11LightPass::reloadShaders();
		break;
	case RenderPassType::Transparent:
		break;
	case RenderPassType::Terrain:
		break;
	case RenderPassType::PostProcessing:
		DX11SkyPass::reloadShaders();
		DX11PreTAAPass::reloadShaders();
		DX11TAAPass::reloadShaders();
		DX11FinalBlendPass::reloadShaders();
		break;
	default: break;
	}

	return true;
}

bool DX11RenderingSystem::bakeGI()
{
	return true;
}
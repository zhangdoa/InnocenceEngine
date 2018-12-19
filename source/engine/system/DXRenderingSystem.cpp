#include "DXRenderingSystem.h"

#include <sstream>

#include "../component/DXGeometryRenderPassComponent.h"
#include "../component/DXLightRenderPassComponent.h"
#include "../component/DXFinalRenderPassComponent.h"

#include "../component/WindowSystemComponent.h"
#include "../component/DXWindowSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/DXRenderingSystemComponent.h"
#include "../component/AssetSystemComponent.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"
#include "../component/DXRenderPassComponent.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../component/GameSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DXRenderingSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

DXRenderPassComponent* addDXRenderPassComponent(unsigned int RTNum, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc, TextureDataDesc RTDesc);

DXMeshDataComponent* generateDXMeshDataComponent(MeshDataComponent* rhs);
DXTextureDataComponent* generateDXTextureDataComponent(TextureDataComponent* rhs);

DXMeshDataComponent* addDXMeshDataComponent(EntityID rhs);
DXTextureDataComponent* addDXTextureDataComponent(EntityID rhs);

DXMeshDataComponent* getDXMeshDataComponent(EntityID rhs);
DXTextureDataComponent* getDXTextureDataComponent(EntityID rhs);

bool setup();
bool terminate();

bool initializeDefaultAssets();
bool initializeGeometryPass();
bool initializeLightPass();
bool initializeFinalBlendPass();

TextureDataDesc deferredPassTextureDesc = TextureDataDesc();
D3D11_RENDER_TARGET_VIEW_DESC deferredPassRTVDesc = D3D11_RENDER_TARGET_VIEW_DESC();

ID3D10Blob* loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath);
void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);

void prepareRenderingData();

void updateGeometryPass();
void updateLightPass();
void updateFinalBlendPass();

void drawMesh(EntityID rhs);
void drawMesh(MeshDataComponent* MDC);
void drawMesh(size_t indicesSize, DXMeshDataComponent * DXMDC);

template <class T>
void updateShaderParameter(ShaderType shaderType, ID3D11Buffer* matrixBuffer, T* parameterValue);

void cleanRTV(vec4 color, ID3D11RenderTargetView* RTV);
void cleanDSV(ID3D11DepthStencilView* DSV);
void swapBuffer();

static WindowSystemComponent* g_WindowSystemComponent;
static DXWindowSystemComponent* g_DXWindowSystemComponent;
static DXRenderingSystemComponent* g_DXRenderingSystemComponent;

std::unordered_map<EntityID, DXMeshDataComponent*> m_initializedMeshComponents;

mat4 m_CamProj;
mat4 m_CamRot;
mat4 m_CamTrans;
mat4 m_CamRTP;

struct GPassCBufferData
{
	mat4 m;
	mat4 vp;
	mat4 m_normalMat;
};

struct GPassRenderingDataPack
{
	size_t indiceSize;
	GPassCBufferData GPassCBuffer;
	DXMeshDataComponent* DXMDC;
	MeshPrimitiveTopology m_meshDrawMethod;
	DXTextureDataComponent* m_basicNormalDXTDC;
	DXTextureDataComponent* m_basicAlbedoDXTDC;
	DXTextureDataComponent* m_basicMetallicDXTDC;
	DXTextureDataComponent* m_basicRoughnessDXTDC;
	DXTextureDataComponent* m_basicAODXTDC;
};

std::queue<GPassRenderingDataPack> m_GPassRenderingDataQueue;

struct LPassCBufferData
{
	vec4 viewPos;
	vec4 lightDir;
	vec4 color;
};

LPassCBufferData m_LPassCBufferData;

DXMeshDataComponent* m_UnitLineTemplate;
DXMeshDataComponent* m_UnitQuadTemplate;
DXMeshDataComponent* m_UnitCubeTemplate;
DXMeshDataComponent* m_UnitSphereTemplate;

DXTextureDataComponent* m_basicNormalTemplate;
DXTextureDataComponent* m_basicAlbedoTemplate;
DXTextureDataComponent* m_basicMetallicTemplate;
DXTextureDataComponent* m_basicRoughnessTemplate;
DXTextureDataComponent* m_basicAOTemplate;
}

bool DXRenderingSystemNS::setup()
{
	g_WindowSystemComponent = &WindowSystemComponent::get();
	g_DXWindowSystemComponent = &DXWindowSystemComponent::get();
	g_DXRenderingSystemComponent = &DXRenderingSystemComponent::get();

	HRESULT result;
	IDXGIFactory* m_factory;

	DXGI_ADAPTER_DESC adapterDesc;
	IDXGIAdapter* m_adapter;
	IDXGIOutput* m_adapterOutput;

	unsigned int numModes, i, numerator, denominator;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;

	int error;
	D3D_FEATURE_LEVEL featureLevel;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_factory);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create DXGI factory!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = m_factory->EnumAdapters(0, &m_adapter);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create video card adapter!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = m_adapter->EnumOutputs(0, &m_adapterOutput);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create monitor adapter!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];

	// Now fill the display mode list structures.
	result = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't fill the display mode list structures!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)g_WindowSystemComponent->m_windowResolution.x
			&&
			displayModeList[i].Height == (unsigned int)g_WindowSystemComponent->m_windowResolution.y
			)
		{
			numerator = displayModeList[i].RefreshRate.Numerator;
			denominator = displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Get the adapter (video card) description.
	result = m_adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't get the video card adapter description!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	g_DXRenderingSystemComponent->m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, g_DXRenderingSystemComponent->m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't convert the name of the video card to a character array!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;

	// Release the adapter output.
	m_adapterOutput->Release();
	m_adapterOutput = 0;

	// Release the adapter.
	m_adapter->Release();
	m_adapter = 0;

	// Release the factory.
	m_factory->Release();
	m_factory = 0;

	// Initialize the swap chain description.
	ZeroMemory(&g_DXRenderingSystemComponent->m_swapChainDesc, sizeof(g_DXRenderingSystemComponent->m_swapChainDesc));

	// Set to a single back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Width = (UINT)g_WindowSystemComponent->m_windowResolution.x;
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Height = (UINT)g_WindowSystemComponent->m_windowResolution.y;

	// Set regular 32-bit surface for the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (g_DXRenderingSystemComponent->m_vsync_enabled)
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	g_DXRenderingSystemComponent->m_swapChainDesc.OutputWindow = g_DXWindowSystemComponent->m_hwnd;

	// Turn multisampling off.
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Count = 1;
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	if (g_WindowSystemComponent->m_fullScreen)
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.Windowed = false;
	}
	else
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.Windowed = true;
	}

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
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &g_DXRenderingSystemComponent->m_swapChainDesc, &g_DXRenderingSystemComponent->m_swapChain, &g_DXRenderingSystemComponent->m_device, NULL, &g_DXRenderingSystemComponent->m_deviceContext);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the swap chain/D3D device/D3D device context!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Get the pointer to the back buffer.
	result = g_DXRenderingSystemComponent->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&g_DXRenderingSystemComponent->m_renderTargetTexture);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't get back buffer pointer!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = g_DXRenderingSystemComponent->m_device->CreateRenderTargetView(g_DXRenderingSystemComponent->m_renderTargetTexture, NULL, &g_DXRenderingSystemComponent->m_renderTargetView);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create render target view!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	g_DXRenderingSystemComponent->m_renderTargetTexture->Release();
	g_DXRenderingSystemComponent->m_renderTargetTexture = 0;

	// Initialize the description of the depth buffer.
	ZeroMemory(&g_DXRenderingSystemComponent->m_depthTextureDesc, sizeof(g_DXRenderingSystemComponent->m_depthTextureDesc));

	// Set up the description of the depth buffer.
	g_DXRenderingSystemComponent->m_depthTextureDesc.Width = (UINT)g_WindowSystemComponent->m_windowResolution.x;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Height = (UINT)g_WindowSystemComponent->m_windowResolution.y;
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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the texture for the depth buffer!");
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
		&g_DXRenderingSystemComponent->m_depthStencilState);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the depth stencil state!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Set the depth stencil state.
	g_DXRenderingSystemComponent->m_deviceContext->OMSetDepthStencilState(
		g_DXRenderingSystemComponent->m_depthStencilState, 1);

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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the depth stencil view!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	g_DXRenderingSystemComponent->m_deviceContext->OMSetRenderTargets(
		1,
		&g_DXRenderingSystemComponent->m_renderTargetView,
		g_DXRenderingSystemComponent->m_depthStencilView);

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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the rasterizer state for forward pass!");
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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the rasterizer state for deferred pass!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Setup the viewport for rendering.
	g_DXRenderingSystemComponent->m_viewport.Width =
		(float)g_WindowSystemComponent->m_windowResolution.x;
	g_DXRenderingSystemComponent->m_viewport.Height =
		(float)g_WindowSystemComponent->m_windowResolution.y;
	g_DXRenderingSystemComponent->m_viewport.MinDepth = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.MaxDepth = 1.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftX = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftY = 0.0f;

	// Setup the description of the deferred pass.
	deferredPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	deferredPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	deferredPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	deferredPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	deferredPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	deferredPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	deferredPassTextureDesc.textureWidth = g_WindowSystemComponent->m_windowResolution.x;
	deferredPassTextureDesc.textureHeight = g_WindowSystemComponent->m_windowResolution.y;
	deferredPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	deferredPassRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	deferredPassRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	deferredPassRTVDesc.Texture2D.MipSlice = 0;

	m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool DXRenderingSystem::setup()
{
	return DXRenderingSystemNS::setup();
}

INNO_SYSTEM_EXPORT bool DXRenderingSystem::initialize()
{
	DXRenderingSystemNS::initializeDefaultAssets();
	DXRenderingSystemNS::initializeGeometryPass();
	DXRenderingSystemNS::initializeLightPass();
	DXRenderingSystemNS::initializeFinalBlendPass();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool DXRenderingSystem::update()
{
	if (AssetSystemComponent::get().m_uninitializedMeshComponents.size() > 0)
	{
		MeshDataComponent* l_meshDataComponent;
		if (AssetSystemComponent::get().m_uninitializedMeshComponents.tryPop(l_meshDataComponent))
		{
			auto l_initializedDXMDC = DXRenderingSystemNS::generateDXMeshDataComponent(l_meshDataComponent);
		}
	}
	if (AssetSystemComponent::get().m_uninitializedTextureComponents.size() > 0)
	{
		TextureDataComponent* l_textureDataComponent;
		if (AssetSystemComponent::get().m_uninitializedTextureComponents.tryPop(l_textureDataComponent))
		{
			DXRenderingSystemNS::generateDXTextureDataComponent(l_textureDataComponent);
		}
	}
	// Clear the buffers to begin the scene.
	DXRenderingSystemNS::prepareRenderingData();

	DXRenderingSystemNS::updateGeometryPass();

	DXRenderingSystemNS::updateLightPass();

	DXRenderingSystemNS::updateFinalBlendPass();

	// Present the rendered scene to the screen.
	DXRenderingSystemNS::swapBuffer();

	return true;
}

bool DXRenderingSystemNS::terminate()
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

	if (g_DXRenderingSystemComponent->m_depthStencilState)
	{
		g_DXRenderingSystemComponent->m_depthStencilState->Release();
		g_DXRenderingSystemComponent->m_depthStencilState = 0;
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
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT bool DXRenderingSystem::terminate()
{
	return DXRenderingSystemNS::terminate();
}

ObjectStatus DXRenderingSystem::getStatus()
{
	return DXRenderingSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool DXRenderingSystem::resize()
{
	return true;
}

bool  DXRenderingSystemNS::initializeDefaultAssets()
{
	std::function<void(MeshDataComponent* MDC)> f_convertCoordinateFromGLtoDX = [&](MeshDataComponent* MDC) {
		for (auto& i : MDC->m_vertices)
		{
		}
	};

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::LINE);
	f_convertCoordinateFromGLtoDX(l_MDC);
	m_UnitLineTemplate = generateDXMeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	f_convertCoordinateFromGLtoDX(l_MDC);
	m_UnitQuadTemplate = generateDXMeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
	f_convertCoordinateFromGLtoDX(l_MDC);
	m_UnitCubeTemplate = generateDXMeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE);
	f_convertCoordinateFromGLtoDX(l_MDC);
	m_UnitSphereTemplate = generateDXMeshDataComponent(l_MDC);

	m_basicNormalTemplate = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::NORMAL));
	m_basicAlbedoTemplate = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO));
	m_basicMetallicTemplate = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::METALLIC));
	m_basicRoughnessTemplate = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	m_basicAOTemplate = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));

	return true;
}

DXRenderPassComponent* DXRenderingSystemNS::addDXRenderPassComponent(unsigned int RTNum, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc, TextureDataDesc RTDesc)
{
	auto l_DXRPC = g_pCoreSystem->getMemorySystem()->spawn<DXRenderPassComponent>();

	HRESULT result;

	// create TDC
	l_DXRPC->m_TDCs.reserve(RTNum);
	
	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

		l_TDC->m_textureDataDesc.textureUsageType = RTDesc.textureUsageType;
		l_TDC->m_textureDataDesc.textureColorComponentsFormat = RTDesc.textureColorComponentsFormat;
		l_TDC->m_textureDataDesc.texturePixelDataFormat = RTDesc.texturePixelDataFormat;
		l_TDC->m_textureDataDesc.textureMinFilterMethod = RTDesc.textureMinFilterMethod;
		l_TDC->m_textureDataDesc.textureMagFilterMethod = RTDesc.textureMagFilterMethod;
		l_TDC->m_textureDataDesc.textureWrapMethod = RTDesc.textureWrapMethod;
		l_TDC->m_textureDataDesc.textureWidth = RTDesc.textureWidth;
		l_TDC->m_textureDataDesc.textureHeight = RTDesc.textureHeight;
		l_TDC->m_textureDataDesc.texturePixelDataType = RTDesc.texturePixelDataType;
		l_TDC->m_textureData = { nullptr };

		l_DXRPC->m_TDCs.emplace_back(l_TDC);
	}

	// generate DXTDC
	l_DXRPC->m_DXTDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = l_DXRPC->m_TDCs[i];
		auto l_DXTDC = generateDXTextureDataComponent(l_TDC);

		l_DXRPC->m_DXTDCs.emplace_back(l_DXTDC);
	}

	// Create the render target views.
	l_DXRPC->m_renderTargetViews.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		l_DXRPC->m_renderTargetViews.emplace_back();
		result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateRenderTargetView(
			l_DXRPC->m_DXTDCs[i]->m_texture,
			&renderTargetViewDesc,
			&l_DXRPC->m_renderTargetViews[i]);
		if (FAILED(result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create render target view!");
			DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
			return false;
		}
	}

	// Initialize the description of the depth buffer.
	ZeroMemory(&l_DXRPC->m_depthBufferDesc,
		sizeof(l_DXRPC->m_depthBufferDesc));

	// Set up the description of the depth buffer.
	l_DXRPC->m_depthBufferDesc.Width = RTDesc.textureWidth;
	l_DXRPC->m_depthBufferDesc.Height =	RTDesc.textureHeight;
	l_DXRPC->m_depthBufferDesc.MipLevels = 1;
	l_DXRPC->m_depthBufferDesc.ArraySize = 1;
	l_DXRPC->m_depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	l_DXRPC->m_depthBufferDesc.SampleDesc.Count = 1;
	l_DXRPC->m_depthBufferDesc.SampleDesc.Quality = 0;
	l_DXRPC->m_depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	l_DXRPC->m_depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	l_DXRPC->m_depthBufferDesc.CPUAccessFlags = 0;
	l_DXRPC->m_depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateTexture2D(
		&l_DXRPC->m_depthBufferDesc,
		NULL,
		&l_DXRPC->m_depthStencilBuffer);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the texture for the depth buffer!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Initailze the depth stencil view description.
	ZeroMemory(&l_DXRPC->m_depthStencilViewDesc,
		sizeof(l_DXRPC->m_depthStencilViewDesc));

	// Set up the depth stencil view description.
	l_DXRPC->m_depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	l_DXRPC->m_depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	l_DXRPC->m_depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateDepthStencilView(
		l_DXRPC->m_depthStencilBuffer,
		&l_DXRPC->m_depthStencilViewDesc,
		&l_DXRPC->m_depthStencilView);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the depth stencil view!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Setup the viewport for rendering.
	l_DXRPC->m_viewport.Width = (float)RTDesc.textureWidth;
	l_DXRPC->m_viewport.Height = (float)RTDesc.textureHeight;
	l_DXRPC->m_viewport.MinDepth = 0.0f;
	l_DXRPC->m_viewport.MaxDepth = 1.0f;
	l_DXRPC->m_viewport.TopLeftX = 0.0f;
	l_DXRPC->m_viewport.TopLeftY = 0.0f;

	l_DXRPC->m_objectStatus = ObjectStatus::ALIVE;

	return l_DXRPC;
}

bool  DXRenderingSystemNS::initializeGeometryPass()
{
	DXGeometryRenderPassComponent::get().m_DXRPC = addDXRenderPassComponent(8, deferredPassRTVDesc, deferredPassTextureDesc);
	DXGeometryRenderPassComponent::get().m_DXSPC = g_pCoreSystem->getMemorySystem()->spawn<DXShaderProgramComponent>();

	HRESULT result;

	ID3D10Blob* l_errorMessage;
	ID3D10Blob* l_shaderBuffer;

	// Initialize the pointers this function will use to null.
	l_errorMessage = 0;
	l_shaderBuffer = 0;

	// Compile the shader code.
	l_shaderBuffer = loadShaderBuffer(ShaderType::VERTEX, L"..//res//shaders//DX11//geometryPassCookTorranceVertex.sf");

	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateVertexShader(
		l_shaderBuffer->GetBufferPointer(), l_shaderBuffer->GetBufferSize(),
		NULL,
		&DXGeometryRenderPassComponent::get().m_DXSPC->m_vertexShader);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: GeometryPass: can't create vertex shader!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC l_polygonLayout[5];
	unsigned int l_numElements;

	// Create the vertex input layout description.
	l_polygonLayout[0].SemanticName = "POSITION";
	l_polygonLayout[0].SemanticIndex = 0;
	l_polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[0].InputSlot = 0;
	l_polygonLayout[0].AlignedByteOffset = 0;
	l_polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[0].InstanceDataStepRate = 0;

	l_polygonLayout[1].SemanticName = "TEXCOORD";
	l_polygonLayout[1].SemanticIndex = 0;
	l_polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[1].InputSlot = 0;
	l_polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[1].InstanceDataStepRate = 0;

	l_polygonLayout[2].SemanticName = "PADA";
	l_polygonLayout[2].SemanticIndex = 0;
	l_polygonLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[2].InputSlot = 0;
	l_polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[2].InstanceDataStepRate = 0;

	l_polygonLayout[3].SemanticName = "NORMAL";
	l_polygonLayout[3].SemanticIndex = 0;
	l_polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[3].InputSlot = 0;
	l_polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[3].InstanceDataStepRate = 0;

	l_polygonLayout[4].SemanticName = "PADB";
	l_polygonLayout[4].SemanticIndex = 0;
	l_polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[4].InputSlot = 0;
	l_polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[4].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	l_numElements = sizeof(l_polygonLayout) / sizeof(l_polygonLayout[0]);

	// Create the vertex input layout.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateInputLayout(
		l_polygonLayout, l_numElements, l_shaderBuffer->GetBufferPointer(),
		l_shaderBuffer->GetBufferSize(), &DXGeometryRenderPassComponent::get().m_DXSPC->m_layout);

	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: GeometryPass: can't create vertex shader layout!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Setup the description of the dynamic matrix constant buffer
	DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.ByteWidth = sizeof(GPassCBufferData);
	DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.MiscFlags = 0;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateBuffer(&DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBufferDesc, NULL, &DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBuffer);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: GeometryPass: can't create constant buffer pointer!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	l_shaderBuffer->Release();
	l_shaderBuffer = 0;

	// Compile the shader code.
	l_shaderBuffer = loadShaderBuffer(ShaderType::FRAGMENT, L"..//res//shaders//DX11//geometryPassCookTorrancePixel.sf");

	// Create the shader from the buffer.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreatePixelShader(
		l_shaderBuffer->GetBufferPointer(),
		l_shaderBuffer->GetBufferSize(),
		NULL,
		&DXGeometryRenderPassComponent::get().m_DXSPC->m_pixelShader);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: GeometryPass: can't create pixel shader!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	l_shaderBuffer->Release();
	l_shaderBuffer = 0;

	// Create a texture sampler state description.
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.MinLOD = 0;
	DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateSamplerState(
		&DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerDesc,
		&DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerState);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: GeometryPass: can't create texture sampler state!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

bool  DXRenderingSystemNS::initializeLightPass()
{
	DXLightRenderPassComponent::get().m_DXRPC = addDXRenderPassComponent(1, deferredPassRTVDesc, deferredPassTextureDesc);
	DXLightRenderPassComponent::get().m_DXSPC = g_pCoreSystem->getMemorySystem()->spawn<DXShaderProgramComponent>();

	HRESULT result;

	ID3D10Blob* l_errorMessage;
	ID3D10Blob* l_shaderBuffer;

	// Initialize the pointers this function will use to null.
	l_errorMessage = 0;
	l_shaderBuffer = 0;

	// Compile the shader code.
	l_shaderBuffer = loadShaderBuffer(ShaderType::VERTEX, L"..//res//shaders//DX11//lightPassCookTorranceVertex.sf");

	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateVertexShader(
		l_shaderBuffer->GetBufferPointer(), l_shaderBuffer->GetBufferSize(),
		NULL,
		&DXLightRenderPassComponent::get().m_DXSPC->m_vertexShader);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: LightPass: can't create vertex shader!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC l_polygonLayout[5];
	unsigned int l_numElements;

	// Create the vertex input layout description.
	l_polygonLayout[0].SemanticName = "POSITION";
	l_polygonLayout[0].SemanticIndex = 0;
	l_polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[0].InputSlot = 0;
	l_polygonLayout[0].AlignedByteOffset = 0;
	l_polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[0].InstanceDataStepRate = 0;

	l_polygonLayout[1].SemanticName = "TEXCOORD";
	l_polygonLayout[1].SemanticIndex = 0;
	l_polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[1].InputSlot = 0;
	l_polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[1].InstanceDataStepRate = 0;

	l_polygonLayout[2].SemanticName = "PADA";
	l_polygonLayout[2].SemanticIndex = 0;
	l_polygonLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[2].InputSlot = 0;
	l_polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[2].InstanceDataStepRate = 0;

	l_polygonLayout[3].SemanticName = "NORMAL";
	l_polygonLayout[3].SemanticIndex = 0;
	l_polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[3].InputSlot = 0;
	l_polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[3].InstanceDataStepRate = 0;

	l_polygonLayout[4].SemanticName = "PADB";
	l_polygonLayout[4].SemanticIndex = 0;
	l_polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[4].InputSlot = 0;
	l_polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[4].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	l_numElements = sizeof(l_polygonLayout) / sizeof(l_polygonLayout[0]);

	// Create the vertex input layout.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateInputLayout(
		l_polygonLayout, l_numElements, l_shaderBuffer->GetBufferPointer(),
		l_shaderBuffer->GetBufferSize(), &DXLightRenderPassComponent::get().m_DXSPC->m_layout);

	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: LightPass: can't create vertex shader layout!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Setup the description of the dynamic matrix constant buffer
	DXLightRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	DXLightRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.ByteWidth = sizeof(LPassCBufferData);
	DXLightRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DXLightRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DXLightRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.MiscFlags = 0;
	DXLightRenderPassComponent::get().m_DXSPC->m_constantBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateBuffer(&DXLightRenderPassComponent::get().m_DXSPC->m_constantBufferDesc, NULL, &DXLightRenderPassComponent::get().m_DXSPC->m_constantBuffer);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: LightPass: can't create constant buffer pointer!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	l_shaderBuffer->Release();
	l_shaderBuffer = 0;

	// Compile the shader code.
	l_shaderBuffer = loadShaderBuffer(ShaderType::FRAGMENT, L"..//res//shaders//DX11//lightPassCookTorrancePixel.sf");

	// Create the shader from the buffer.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreatePixelShader(
		l_shaderBuffer->GetBufferPointer(),
		l_shaderBuffer->GetBufferSize(),
		NULL,
		&DXLightRenderPassComponent::get().m_DXSPC->m_pixelShader);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: LightPass: can't create pixel shader!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	l_shaderBuffer->Release();
	l_shaderBuffer = 0;

	// Create a texture sampler state description.
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.MinLOD = 0;
	DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateSamplerState(
		&DXLightRenderPassComponent::get().m_DXSPC->m_samplerDesc,
		&DXLightRenderPassComponent::get().m_DXSPC->m_samplerState);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: LightPass: can't create texture sampler state!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

bool DXRenderingSystemNS::initializeFinalBlendPass()
{
	HRESULT result;
	ID3D10Blob* l_errorMessage;
	ID3D10Blob* l_shaderBuffer;

	// Initialize the pointers this function will use to null.
	l_errorMessage = 0;
	l_shaderBuffer = 0;

	// Compile the shader code.
	l_shaderBuffer = loadShaderBuffer(ShaderType::VERTEX, L"..//res//shaders//DX11//finalBlendPassVertex.sf");

	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateVertexShader(
		l_shaderBuffer->GetBufferPointer(),
		l_shaderBuffer->GetBufferSize(),
		NULL,
		&DXFinalRenderPassComponent::get().m_vertexShader);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: FinalBlendPass: can't create vertex shader!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC l_polygonLayout[5];
	unsigned int l_numElements;

	// Create the vertex input layout description.
	l_polygonLayout[0].SemanticName = "POSITION";
	l_polygonLayout[0].SemanticIndex = 0;
	l_polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[0].InputSlot = 0;
	l_polygonLayout[0].AlignedByteOffset = 0;
	l_polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[0].InstanceDataStepRate = 0;

	l_polygonLayout[1].SemanticName = "TEXCOORD";
	l_polygonLayout[1].SemanticIndex = 0;
	l_polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[1].InputSlot = 0;
	l_polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[1].InstanceDataStepRate = 0;

	l_polygonLayout[2].SemanticName = "PADA";
	l_polygonLayout[2].SemanticIndex = 0;
	l_polygonLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[2].InputSlot = 0;
	l_polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[2].InstanceDataStepRate = 0;

	l_polygonLayout[3].SemanticName = "NORMAL";
	l_polygonLayout[3].SemanticIndex = 0;
	l_polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[3].InputSlot = 0;
	l_polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[3].InstanceDataStepRate = 0;

	l_polygonLayout[4].SemanticName = "PADB";
	l_polygonLayout[4].SemanticIndex = 0;
	l_polygonLayout[4].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[4].InputSlot = 0;
	l_polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	l_polygonLayout[4].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	l_numElements = sizeof(l_polygonLayout) / sizeof(l_polygonLayout[0]);

	// Create the vertex input layout.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateInputLayout(
		l_polygonLayout, l_numElements, l_shaderBuffer->GetBufferPointer(),
		l_shaderBuffer->GetBufferSize(), &DXFinalRenderPassComponent::get().m_layout);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: FinalBlendPass: can't create vertex shader layout!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	l_shaderBuffer->Release();
	l_shaderBuffer = 0;

	// Compile the shader code.
	l_shaderBuffer = loadShaderBuffer(ShaderType::FRAGMENT, L"..//res//shaders//DX11//finalBlendPassPixel.sf");

	// Create the shader from the buffer.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreatePixelShader(
		l_shaderBuffer->GetBufferPointer(),
		l_shaderBuffer->GetBufferSize(),
		NULL,
		&DXFinalRenderPassComponent::get().m_pixelShader);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: FinalBlendPass: can't create pixel shader!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	l_shaderBuffer->Release();
	l_shaderBuffer = 0;

	// Create a texture sampler state description.
	DXFinalRenderPassComponent::get().m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	DXFinalRenderPassComponent::get().m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXFinalRenderPassComponent::get().m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXFinalRenderPassComponent::get().m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXFinalRenderPassComponent::get().m_samplerDesc.MipLODBias = 0.0f;
	DXFinalRenderPassComponent::get().m_samplerDesc.MaxAnisotropy = 1;
	DXFinalRenderPassComponent::get().m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	DXFinalRenderPassComponent::get().m_samplerDesc.BorderColor[0] = 0;
	DXFinalRenderPassComponent::get().m_samplerDesc.BorderColor[1] = 0;
	DXFinalRenderPassComponent::get().m_samplerDesc.BorderColor[2] = 0;
	DXFinalRenderPassComponent::get().m_samplerDesc.BorderColor[3] = 0;
	DXFinalRenderPassComponent::get().m_samplerDesc.MinLOD = 0;
	DXFinalRenderPassComponent::get().m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateSamplerState(
		&DXFinalRenderPassComponent::get().m_samplerDesc,
		&DXFinalRenderPassComponent::get().m_samplerState);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: FinalBlendPass: can't create texture sampler state!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

ID3D10Blob * DXRenderingSystemNS::loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath)
{
	auto l_shaderFilePath = shaderFilePath.c_str();
	auto l_shaderName = std::string(shaderFilePath.begin(), shaderFilePath.end());
	std::reverse(l_shaderName.begin(), l_shaderName.end());
	l_shaderName = l_shaderName.substr(l_shaderName.find(".") + 1, l_shaderName.find("//") - l_shaderName.find(".") - 1);
	std::reverse(l_shaderName.begin(), l_shaderName.end());

	HRESULT result;
	ID3D10Blob* l_errorMessage;
	ID3D10Blob* l_shaderBuffer;

	std::string l_shaderTypeName;

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		l_shaderTypeName = "vs_5_0";
		break;
	case ShaderType::GEOMETRY:
		l_shaderTypeName = "gs_5_0";
		break;
	case ShaderType::FRAGMENT:
		l_shaderTypeName = "ps_5_0";
		break;
	default:
		break;
	}

	result = D3DCompileFromFile(l_shaderFilePath, NULL, NULL, l_shaderName.c_str(), l_shaderTypeName.c_str(), D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&l_shaderBuffer, &l_errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (l_errorMessage)
		{
			OutputShaderErrorMessage(l_errorMessage, DXRenderingSystemNS::g_DXWindowSystemComponent->m_hwnd, l_shaderName.c_str());
		}
		// If there was nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(DXRenderingSystemNS::g_DXWindowSystemComponent->m_hwnd, l_shaderName.c_str(), "Missing Shader File", MB_OK);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "Shader creation failed: cannot find shader!");
		}

		return nullptr;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DXRenderingSystem: innoShader: " + l_shaderName + " Shader has been compiled.");
	return l_shaderBuffer;
}

void DXRenderingSystemNS::OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename)
{
	char* compileErrors;
	unsigned long long bufferSize, i;
	std::stringstream errorSStream;

	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		errorSStream << compileErrors[i];
	}

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	MessageBox(DXRenderingSystemNS::g_DXWindowSystemComponent->m_hwnd, errorSStream.str().c_str(), shaderFilename.c_str(), MB_OK);
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: innoShader: " + shaderFilename + " compile error: " + errorSStream.str() + "\n -- --------------------------------------------------- -- ");
}

DXMeshDataComponent* DXRenderingSystemNS::generateDXMeshDataComponent(MeshDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return DXRenderingSystemNS::getDXMeshDataComponent(rhs->m_parentEntity);
	}
	else
	{
		auto l_ptr = addDXMeshDataComponent(rhs->m_parentEntity);

		// Set up the description of the static vertex buffer.
		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Vertex) * (UINT)rhs->m_vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		// Give the subresource structure a pointer to the vertex data.
		// @TODO: InnoMath's vec4 is 32bit while XMFLOAT4 is 16bit
		D3D11_SUBRESOURCE_DATA vertexData;
		ZeroMemory(&vertexData, sizeof(vertexData));
		vertexData.pSysMem = &rhs->m_vertices[0];
		vertexData.SysMemPitch = 0;
		vertexData.SysMemSlicePitch = 0;

		// Now create the vertex buffer.
		HRESULT result;
		result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &l_ptr->m_vertexBuffer);
		if (FAILED(result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create vbo!");
			return nullptr;
		}

		// Set up the description of the static index buffer.
		D3D11_BUFFER_DESC indexBufferDesc;
		ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(unsigned long) * (UINT)rhs->m_indices.size();
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		// Give the subresource structure a pointer to the index data.
		D3D11_SUBRESOURCE_DATA indexData;
		ZeroMemory(&indexData, sizeof(indexData));
		indexData.pSysMem = &rhs->m_indices[0];
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		// Create the index buffer.
		result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateBuffer(&indexBufferDesc, &indexData, &l_ptr->m_indexBuffer);
		if (FAILED(result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create ibo!");
			return nullptr;
		}
		l_ptr->m_objectStatus = ObjectStatus::ALIVE;
		rhs->m_objectStatus = ObjectStatus::ALIVE;

		DXRenderingSystemNS::m_initializedMeshComponents.emplace(l_ptr->m_parentEntity, l_ptr);
		return l_ptr;
	}
}

bool initializeDXTextureDataComponent(DXTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData)
{
	// set texture formats
	DXGI_FORMAT l_internalFormat = DXGI_FORMAT_UNKNOWN;

	// @TODO: Unified internal format
	// Setup the description of the texture.
	// Different than OpenGL, DX's format didn't allow a RGB structure for 8-bits and 16-bits per channel
	if (textureDataDesc.textureUsageType == TextureUsageType::ALBEDO)
	{
		l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else
	{
		if (textureDataDesc.texturePixelDataType == TexturePixelDataType::UNSIGNED_BYTE)
		{
			switch (textureDataDesc.texturePixelDataFormat)
			{
			case TexturePixelDataFormat::RED: l_internalFormat = DXGI_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.texturePixelDataType == TexturePixelDataType::FLOAT)
		{
			switch (textureDataDesc.texturePixelDataFormat)
			{
			case TexturePixelDataFormat::RED: l_internalFormat = DXGI_FORMAT_R16_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			default: break;
			}
		}
	}

	unsigned int textureMipLevels = 1;
	unsigned int miscFlags = 0;
	if (textureDataDesc.textureMagFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		textureMipLevels = 0;
		miscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	D3D11_TEXTURE2D_DESC D3DTextureDesc;
	ZeroMemory(&D3DTextureDesc, sizeof(D3DTextureDesc));
	D3DTextureDesc.Height = textureDataDesc.textureHeight;
	D3DTextureDesc.Width = textureDataDesc.textureWidth;
	D3DTextureDesc.MipLevels = textureMipLevels;
	D3DTextureDesc.ArraySize = 1;
	D3DTextureDesc.Format = l_internalFormat;
	D3DTextureDesc.SampleDesc.Count = 1;
	if (textureDataDesc.textureUsageType != TextureUsageType::RENDER_TARGET)
	{
		D3DTextureDesc.SampleDesc.Quality = 0;
	}
	D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	D3DTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	D3DTextureDesc.CPUAccessFlags = 0;
	D3DTextureDesc.MiscFlags = miscFlags;

	unsigned int SRVMipLevels = -1;
	if (textureDataDesc.textureUsageType == TextureUsageType::RENDER_TARGET)
	{
		SRVMipLevels = 1;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = D3DTextureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = SRVMipLevels;

	// Create the empty texture.
	HRESULT hResult;
	hResult = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateTexture2D(&D3DTextureDesc, NULL, &rhs->m_texture);
	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create texture!");
		return nullptr;
	}

	if (textureDataDesc.textureUsageType != TextureUsageType::RENDER_TARGET)
	{
		unsigned int rowPitch;
		rowPitch = (textureDataDesc.textureWidth * 4) * sizeof(unsigned char);
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->UpdateSubresource(rhs->m_texture, 0, NULL, textureData[0], rowPitch, 0);
	}

	// Create the shader resource view for the texture.
	hResult = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_device->CreateShaderResourceView(rhs->m_texture, &srvDesc, &rhs->m_SRV);
	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create shader resource view for texture!");
		return nullptr;
	}

	// Generate mipmaps for this texture.
	if (textureDataDesc.textureMagFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->GenerateMips(rhs->m_SRV);
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	return rhs;
}

DXTextureDataComponent* DXRenderingSystemNS::generateDXTextureDataComponent(TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getDXTextureDataComponent(rhs->m_parentEntity);
	}
	else
	{
		if (rhs->m_textureDataDesc.textureUsageType == TextureUsageType::INVISIBLE)
		{
			return nullptr;
		}
		else
		{
			auto l_ptr = addDXTextureDataComponent(rhs->m_parentEntity);

			initializeDXTextureDataComponent(l_ptr, rhs->m_textureDataDesc, rhs->m_textureData);

			rhs->m_objectStatus = ObjectStatus::ALIVE;

			return l_ptr;
		}
	}
}

DXMeshDataComponent* DXRenderingSystemNS::addDXMeshDataComponent(EntityID rhs)
{
	DXMeshDataComponent* newMesh = g_pCoreSystem->getMemorySystem()->spawn<DXMeshDataComponent>();
	newMesh->m_parentEntity = rhs;
	auto l_meshMap = &DXRenderingSystemNS::g_DXRenderingSystemComponent->m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, DXMeshDataComponent*>(rhs, newMesh));
	return newMesh;
}

DXTextureDataComponent* DXRenderingSystemNS::addDXTextureDataComponent(EntityID rhs)
{
	DXTextureDataComponent* newTexture = g_pCoreSystem->getMemorySystem()->spawn<DXTextureDataComponent>();
	newTexture->m_parentEntity = rhs;
	auto l_textureMap = &DXRenderingSystemNS::g_DXRenderingSystemComponent->m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, DXTextureDataComponent*>(rhs, newTexture));
	return newTexture;
}

DXMeshDataComponent * DXRenderingSystemNS::getDXMeshDataComponent(EntityID rhs)
{
	auto result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_meshMap.find(rhs);
	if (result != DXRenderingSystemNS::g_DXRenderingSystemComponent->m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

DXTextureDataComponent * DXRenderingSystemNS::getDXTextureDataComponent(EntityID rhs)
{
	auto result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_textureMap.find(rhs);
	if (result != DXRenderingSystemNS::g_DXRenderingSystemComponent->m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

void DXRenderingSystemNS::prepareRenderingData()
{
	// camera and light
	auto l_mainCamera = GameSystemComponent::get().m_CameraComponents[0];
	auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);
	auto l_directionalLight = GameSystemComponent::get().m_DirectionalLightComponents[0];
	auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

	auto l_p = l_mainCamera->m_projectionMatrix;
	auto l_r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);
	auto l_t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);

	DXRenderingSystemNS::m_CamProj = l_p;
	DXRenderingSystemNS::m_CamRot = l_r;
	DXRenderingSystemNS::m_CamTrans = l_t;
	DXRenderingSystemNS::m_CamRTP = l_p * l_r * l_t;

	m_LPassCBufferData.viewPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;
	m_LPassCBufferData.lightDir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	m_LPassCBufferData.color = l_directionalLight->m_color;

	for (auto& l_renderDataPack : RenderingSystemComponent::get().m_renderDataPack)
	{
		auto l_DXMDC = DXRenderingSystemNS::m_initializedMeshComponents.find(l_renderDataPack.MDC->m_parentEntity);
		if (l_DXMDC != DXRenderingSystemNS::m_initializedMeshComponents.end())
		{
			GPassRenderingDataPack l_renderingDataPack;

			l_renderingDataPack.indiceSize = l_renderDataPack.MDC->m_indicesSize;
			l_renderingDataPack.m_meshDrawMethod = l_renderDataPack.MDC->m_meshDrawMethod;
			l_renderingDataPack.GPassCBuffer.m = l_renderDataPack.m;
			l_renderingDataPack.GPassCBuffer.vp = DXRenderingSystemNS::m_CamRTP;
			l_renderingDataPack.GPassCBuffer.m_normalMat = l_renderDataPack.normalMat;
			l_renderingDataPack.DXMDC = l_DXMDC->second;
			// any normal?
			auto l_TDC = l_renderDataPack.Material->m_texturePack.m_normalTDC.second;
			if (l_TDC)
			{
				l_renderingDataPack.m_basicNormalDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_renderingDataPack.m_basicNormalDXTDC = m_basicNormalTemplate;
			}
			// any albedo?
			l_TDC = l_renderDataPack.Material->m_texturePack.m_albedoTDC.second;
			if (l_TDC)
			{
				l_renderingDataPack.m_basicAlbedoDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_renderingDataPack.m_basicAlbedoDXTDC = m_basicAlbedoTemplate;
			}
			// any metallic?
			l_TDC = l_renderDataPack.Material->m_texturePack.m_metallicTDC.second;
			if (l_TDC)
			{
				l_renderingDataPack.m_basicMetallicDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_renderingDataPack.m_basicMetallicDXTDC = m_basicMetallicTemplate;
			}
			// any roughness?
			l_TDC = l_renderDataPack.Material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC)
			{
				l_renderingDataPack.m_basicRoughnessDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_renderingDataPack.m_basicRoughnessDXTDC = m_basicRoughnessTemplate;
			}
			// any ao?
			l_TDC = l_renderDataPack.Material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC)
			{
				l_renderingDataPack.m_basicAODXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_renderingDataPack.m_basicAODXTDC = m_basicAOTemplate;
			}
			DXRenderingSystemNS::m_GPassRenderingDataQueue.push(l_renderingDataPack);
		}
	}
}

void DXRenderingSystemNS::updateGeometryPass()
{
	// Set Rasterizer State
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->RSSetState(
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_rasterStateForward);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->VSSetShader(
		DXGeometryRenderPassComponent::get().m_DXSPC->m_vertexShader,
		NULL,
		0);
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShader(
		DXGeometryRenderPassComponent::get().m_DXSPC->m_pixelShader,
		NULL,
		0);

	// Set the sampler state in the pixel shader.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetSamplers(0, 1, &DXGeometryRenderPassComponent::get().m_DXSPC->m_samplerState);

	// Set the vertex input layout.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetInputLayout(DXGeometryRenderPassComponent::get().m_DXSPC->m_layout);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->OMSetRenderTargets(
		(unsigned int)DXGeometryRenderPassComponent::get().m_DXRPC->m_renderTargetViews.size(),
		&DXGeometryRenderPassComponent::get().m_DXRPC->m_renderTargetViews[0],
		DXGeometryRenderPassComponent::get().m_DXRPC->m_depthStencilView);

	// Set the viewport.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->RSSetViewports(
		1,
		&DXGeometryRenderPassComponent::get().m_DXRPC->m_viewport);

	// Clear the render buffers.
	for (auto i : DXGeometryRenderPassComponent::get().m_DXRPC->m_renderTargetViews)
	{
		DXRenderingSystemNS::cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), i);
	}
	DXRenderingSystemNS::cleanDSV(DXGeometryRenderPassComponent::get().m_DXRPC->m_depthStencilView);

	// draw
	while (DXRenderingSystemNS::m_GPassRenderingDataQueue.size() > 0)
	{
		auto l_renderPack = DXRenderingSystemNS::m_GPassRenderingDataQueue.front();

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		D3D_PRIMITIVE_TOPOLOGY l_primitiveTopology;

		if (l_renderPack.m_meshDrawMethod == MeshPrimitiveTopology::TRIANGLE)
		{
			l_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
		else
		{
			l_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		}

		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetPrimitiveTopology(l_primitiveTopology);

		updateShaderParameter<GPassCBufferData>(ShaderType::VERTEX, DXGeometryRenderPassComponent::get().m_DXSPC->m_constantBuffer, &l_renderPack.GPassCBuffer);

		// bind to textures
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(0, 1, &l_renderPack.m_basicNormalDXTDC->m_SRV);
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(1, 1, &l_renderPack.m_basicAlbedoDXTDC->m_SRV);
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(2, 1, &l_renderPack.m_basicMetallicDXTDC->m_SRV);
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(3, 1, &l_renderPack.m_basicRoughnessDXTDC->m_SRV);
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(4, 1, &l_renderPack.m_basicAODXTDC->m_SRV);

		drawMesh(l_renderPack.indiceSize, l_renderPack.DXMDC);

		DXRenderingSystemNS::m_GPassRenderingDataQueue.pop();
	}
}


void DXRenderingSystemNS::updateLightPass()
{
	// Set Rasterizer State
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->RSSetState(
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_rasterStateDeferred);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->VSSetShader(
		DXLightRenderPassComponent::get().m_DXSPC->m_vertexShader,
		NULL,
		0);
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShader(
		DXLightRenderPassComponent::get().m_DXSPC->m_pixelShader,
		NULL,
		0);

	// Set the sampler state in the pixel shader.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetSamplers(0, 1, &DXLightRenderPassComponent::get().m_DXSPC->m_samplerState);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set the vertex input layout.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetInputLayout(DXLightRenderPassComponent::get().m_DXSPC->m_layout);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->OMSetRenderTargets(
		(unsigned int)DXLightRenderPassComponent::get().m_DXRPC->m_renderTargetViews.size(),
		&DXLightRenderPassComponent::get().m_DXRPC->m_renderTargetViews[0],
		DXLightRenderPassComponent::get().m_DXRPC->m_depthStencilView);

	// Set the viewport.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->RSSetViewports(
		1,
		&DXLightRenderPassComponent::get().m_DXRPC->m_viewport);

	// Clear the render buffers.
	// Clear the render buffers.
	for (auto i : DXLightRenderPassComponent::get().m_DXRPC->m_renderTargetViews)
	{
		DXRenderingSystemNS::cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), i);
	}
	DXRenderingSystemNS::cleanDSV(DXLightRenderPassComponent::get().m_DXRPC->m_depthStencilView);

	auto l_LPassCBufferData = DXRenderingSystemNS::m_LPassCBufferData;

	updateShaderParameter<LPassCBufferData>(ShaderType::FRAGMENT, DXLightRenderPassComponent::get().m_DXSPC->m_constantBuffer, &l_LPassCBufferData);

	// bind to previous pass render target textures
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(0, 1, &DXGeometryRenderPassComponent::get().m_DXRPC->m_DXTDCs[0]->m_SRV);
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(1, 1, &DXGeometryRenderPassComponent::get().m_DXRPC->m_DXTDCs[1]->m_SRV);
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(2, 1, &DXGeometryRenderPassComponent::get().m_DXRPC->m_DXTDCs[2]->m_SRV);

	// draw
	drawMesh(6, m_UnitQuadTemplate);
}

void DXRenderingSystemNS::updateFinalBlendPass()
{
	// Set Rasterizer State
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->RSSetState(
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_rasterStateDeferred);
	// Set the vertex and pixel shaders that will be used to render this triangle.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->VSSetShader(DXFinalRenderPassComponent::get().m_vertexShader, NULL, 0);
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShader(DXFinalRenderPassComponent::get().m_pixelShader, NULL, 0);

	// Set the sampler state in the pixel shader.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetSamplers(0, 1, &DXFinalRenderPassComponent::get().m_samplerState);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set the vertex input layout.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetInputLayout(DXFinalRenderPassComponent::get().m_layout);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->OMSetRenderTargets(
		1,
		&DXRenderingSystemNS::g_DXRenderingSystemComponent->m_renderTargetView,
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_depthStencilView);

	// Set the viewport.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->RSSetViewports(
		1,
		&DXRenderingSystemNS::g_DXRenderingSystemComponent->m_viewport);

	// Clear the render buffers.
	DXRenderingSystemNS::cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), DXRenderingSystemNS::g_DXRenderingSystemComponent->m_renderTargetView);
	DXRenderingSystemNS::cleanDSV(DXRenderingSystemNS::g_DXRenderingSystemComponent->m_depthStencilView);

	// bind to previous pass render target textures
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetShaderResources(0, 1, &DXLightRenderPassComponent::get().m_DXRPC->m_DXTDCs[0]->m_SRV);

	// draw
	drawMesh(6, m_UnitQuadTemplate);
}

void DXRenderingSystemNS::drawMesh(EntityID rhs)
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(rhs);
	if (l_MDC)
	{
		drawMesh(l_MDC);
	}
}

void DXRenderingSystemNS::drawMesh(MeshDataComponent * MDC)
{
	auto l_DXMDC = DXRenderingSystemNS::getDXMeshDataComponent(MDC->m_parentEntity);
	if (l_DXMDC)
	{
		if (MDC->m_objectStatus == ObjectStatus::ALIVE && l_DXMDC->m_objectStatus == ObjectStatus::ALIVE)
		{
			drawMesh(MDC->m_indicesSize, l_DXMDC);
		}
	}
}

void DXRenderingSystemNS::drawMesh(size_t indicesSize, DXMeshDataComponent * DXMDC)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(Vertex);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetVertexBuffers(0, 1, &DXMDC->m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->IASetIndexBuffer(DXMDC->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Render the triangle.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->DrawIndexed((UINT)indicesSize, 0, 0);
}

template <class T>
void DXRenderingSystemNS::updateShaderParameter(ShaderType shaderType, ID3D11Buffer * matrixBuffer, T* parameterValue)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	unsigned int bufferNumber;

	// Lock the constant buffer so it can be written to.
	result = DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't lock the shader matrix buffer!");
		DXRenderingSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return;
	}

	auto dataPtr = reinterpret_cast<T*>(mappedResource.pData);

	*dataPtr = *parameterValue;

	// Unlock the constant buffer.
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->Unmap(matrixBuffer, 0);

	bufferNumber = 0;

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->VSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);
		break;
	case ShaderType::GEOMETRY:
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->GSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);
		break;
	case ShaderType::FRAGMENT:
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->PSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);
		break;
	default:
		break;
	}
}

void DXRenderingSystemNS::cleanRTV(vec4 color, ID3D11RenderTargetView* RTV)
{
	float l_color[4];

	// Setup the color to clear the buffer to.
	l_color[0] = color.x;
	l_color[1] = color.y;
	l_color[2] = color.z;
	l_color[3] = color.w;

	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->ClearRenderTargetView(RTV, l_color);
}

void DXRenderingSystemNS::cleanDSV(ID3D11DepthStencilView* DSV)
{
	DXRenderingSystemNS::g_DXRenderingSystemComponent->m_deviceContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DXRenderingSystemNS::swapBuffer()
{
	// Present the back buffer to the screen since rendering is complete.
	if (DXRenderingSystemNS::g_DXRenderingSystemComponent->m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		DXRenderingSystemNS::g_DXRenderingSystemComponent->m_swapChain->Present(0, 0);
	}
}
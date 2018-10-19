#include "DXRenderingSystem.h"

#include "../../component/DXFinalRenderPassSingletonComponent.h"

#include <sstream>

#include "../../component/RenderingSystemSingletonComponent.h"
#include "../../component/WindowSystemSingletonComponent.h"
#include "../../component/DXWindowSystemSingletonComponent.h"
#include "../../component/AssetSystemSingletonComponent.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#include "DXHeaders.h"

#include "../LowLevelSystem/LogSystem.h"
#include "../HighLevelSystem/AssetSystem.h"
#include "../HighLevelSystem/GameSystem.h"
#include "../../component/GameSystemSingletonComponent.h"

namespace DXRenderingSystem
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	bool m_vsync_enabled;
	int m_videoCardMemory;
	char m_videoCardDescription[128];
	IDXGISwapChain* m_swapChain;
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;
	ID3D11RenderTargetView* m_renderTargetView;
	ID3D11Texture2D* m_depthStencilBuffer;
	ID3D11DepthStencilState* m_depthStencilState;
	ID3D11DepthStencilView* m_depthStencilView;
	ID3D11RasterizerState* m_rasterState;

	void initializeMesh(MeshDataComponent* DXMeshDataComponent);
	void initializeTexture(TextureDataComponent* DXTextureDataComponent);

	void initializeFinalBlendPass();

	void initializeShader(shaderType shaderType, const std::wstring & shaderFilePath);
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);

	void updateFinalBlendPass();

	void updateShaderParameter(shaderType shaderType, ID3D11Buffer* matrixBuffer, mat4* parameterValue);
	void beginScene(float r, float g, float b, float a);
	void endScene();
};

InnoHighLevelSystem_EXPORT bool DXRenderingSystem::Instance::setup()
{
	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	D3D11_VIEWPORT viewport;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create DXGI factory!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create video card adapter!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create monitor adapter!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't fill the display mode list structures!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (i = 0; i<numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)WindowSystemSingletonComponent::getInstance().m_windowResolution.x)
		{
			if (displayModeList[i].Height == (unsigned int)WindowSystemSingletonComponent::getInstance().m_windowResolution.y)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't get the video card adapter description!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't convert the name of the video card to a character array!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = 0;

	// Release the adapter.
	adapter->Release();
	adapter = 0;

	// Release the factory.
	factory->Release();
	factory = 0;

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = (UINT)WindowSystemSingletonComponent::getInstance().m_windowResolution.x;
	swapChainDesc.BufferDesc.Height = (UINT)WindowSystemSingletonComponent::getInstance().m_windowResolution.y;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = DXWindowSystemSingletonComponent::getInstance().m_hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	if (WindowSystemSingletonComponent::getInstance().m_fullScreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, NULL, &m_deviceContext);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create the swap chain/D3D device/D3D device context!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Get the pointer to the back buffer.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't get back buffer pointer!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = m_device->CreateRenderTargetView(backBufferPtr, NULL, &m_renderTargetView);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create render target view!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = (UINT)WindowSystemSingletonComponent::getInstance().m_windowResolution.x;
	depthBufferDesc.Height = (UINT)WindowSystemSingletonComponent::getInstance().m_windowResolution.y;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create the texture for the depth buffer!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create the depth stencil state!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Set the depth stencil state.
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	// Initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create the depth stencil view!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create the rasterizer state!");
		m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	// Now set the rasterizer state.
	m_deviceContext->RSSetState(m_rasterState);

	// Setup the viewport for rendering.
	viewport.Width = (float)WindowSystemSingletonComponent::getInstance().m_windowResolution.x;
	viewport.Height = (float)WindowSystemSingletonComponent::getInstance().m_windowResolution.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_deviceContext->RSSetViewports(1, &viewport);

	m_objectStatus = objectStatus::ALIVE;
	return true;
}

InnoHighLevelSystem_EXPORT bool DXRenderingSystem::Instance::initialize()
{
	initializeFinalBlendPass();

	InnoLogSystem::printLog("DXRenderingSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool DXRenderingSystem::Instance::update()
{
	if (AssetSystemSingletonComponent::getInstance().m_uninitializedMeshComponents.size() > 0)
	{
		MeshDataComponent* l_meshDataComponent;
		if (AssetSystemSingletonComponent::getInstance().m_uninitializedMeshComponents.tryPop(l_meshDataComponent))
		{
			initializeMesh(l_meshDataComponent);
		}
	}
	if (AssetSystemSingletonComponent::getInstance().m_uninitializedTextureComponents.size() > 0)
	{
		TextureDataComponent* l_textureDataComponent;
		if (AssetSystemSingletonComponent::getInstance().m_uninitializedTextureComponents.tryPop(l_textureDataComponent))
		{
			//initializeTexture(l_textureDataComponent);
		}
	}
	// Clear the buffers to begin the scene.
	beginScene(0.0f, 0.0f, 0.0f, 0.0f);

	updateFinalBlendPass();

	// Present the rendered scene to the screen.
	endScene();
	return true;
}

InnoHighLevelSystem_EXPORT bool DXRenderingSystem::Instance::terminate()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	if (m_rasterState)
	{
		m_rasterState->Release();
		m_rasterState = 0;
	}

	if (m_depthStencilView)
	{
		m_depthStencilView->Release();
		m_depthStencilView = 0;
	}

	if (m_depthStencilState)
	{
		m_depthStencilState->Release();
		m_depthStencilState = 0;
	}

	if (m_depthStencilBuffer)
	{
		m_depthStencilBuffer->Release();
		m_depthStencilBuffer = 0;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = 0;
	}

	if (m_deviceContext)
	{
		m_deviceContext->Release();
		m_deviceContext = 0;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	m_objectStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("DXRenderingSystem has been terminated.");
	return true;
}

objectStatus DXRenderingSystem::Instance::getStatus()
{
	return m_objectStatus;
}

void DXRenderingSystem::initializeFinalBlendPass()
{
	initializeShader(shaderType::VERTEX, L"..//res//shaders//DX11//testVertex.sf");

	initializeShader(shaderType::FRAGMENT, L"..//res//shaders//DX11//testPixel.sf");

	// Setup the description of the dynamic matrix constant buffer
	DXFinalRenderPassSingletonComponent::getInstance().m_matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	DXFinalRenderPassSingletonComponent::getInstance().m_matrixBufferDesc.ByteWidth = sizeof(DirectX::XMMATRIX);
	DXFinalRenderPassSingletonComponent::getInstance().m_matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DXFinalRenderPassSingletonComponent::getInstance().m_matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DXFinalRenderPassSingletonComponent::getInstance().m_matrixBufferDesc.MiscFlags = 0;
	DXFinalRenderPassSingletonComponent::getInstance().m_matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer
	auto result = m_device->CreateBuffer(&DXFinalRenderPassSingletonComponent::getInstance().m_matrixBufferDesc, NULL, &DXFinalRenderPassSingletonComponent::getInstance().m_matrixBuffer);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create matrix buffer pointer!");
		m_objectStatus = objectStatus::STANDBY;
		return;
	}

	// Create a texture sampler state description.
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.MipLODBias = 0.0f;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.MaxAnisotropy = 1;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.BorderColor[0] = 0;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.BorderColor[1] = 0;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.BorderColor[2] = 0;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.BorderColor[3] = 0;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.MinLOD = 0;
	DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = m_device->CreateSamplerState(&DXFinalRenderPassSingletonComponent::getInstance().m_samplerDesc, &DXFinalRenderPassSingletonComponent::getInstance().m_sampleState);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create texture sampler state!");
		m_objectStatus = objectStatus::STANDBY;
		return;
	}
}

void DXRenderingSystem::initializeShader(shaderType shaderType, const std::wstring & shaderFilePath)
{
	auto l_shaderFilePath = shaderFilePath.c_str();
	auto l_shaderName = std::string(shaderFilePath.begin(), shaderFilePath.end());
	std::reverse(l_shaderName.begin(), l_shaderName.end());
	l_shaderName = l_shaderName.substr(l_shaderName.find(".") + 1, l_shaderName.find("//") - l_shaderName.find(".") - 1);
	std::reverse(l_shaderName.begin(), l_shaderName.end());

	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* shaderBuffer;

	// Initialize the pointers this function will use to null.
	errorMessage = 0;
	shaderBuffer = 0;

	switch (shaderType)
	{
	case shaderType::VERTEX:
		// Compile the shader code.
		result = D3DCompileFromFile(l_shaderFilePath, NULL, NULL, l_shaderName.c_str(), "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
			&shaderBuffer, &errorMessage);
		if (FAILED(result))
		{
			// If the shader failed to compile it should have writen something to the error message.
			if (errorMessage)
			{
				OutputShaderErrorMessage(errorMessage, DXWindowSystemSingletonComponent::getInstance().m_hwnd, l_shaderName.c_str());
			}
			// If there was nothing in the error message then it simply could not find the shader file itself.
			else
			{
				MessageBox(DXWindowSystemSingletonComponent::getInstance().m_hwnd, l_shaderName.c_str(), "Missing Shader File", MB_OK);
				InnoLogSystem::printLog("Error: Shader creation failed: cannot find shader!");
			}

			return;
		}
		InnoLogSystem::printLog("DXRenderingSystem: innoShader: " + l_shaderName + " Shader has been compiled.");

		// Create the shader from the buffer.
		result = m_device->CreateVertexShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, &DXFinalRenderPassSingletonComponent::getInstance().m_vertexShader);
		if (FAILED(result))
		{
			InnoLogSystem::printLog("Error: DXRenderingSystem: can't create vertex shader!");
			m_objectStatus = objectStatus::STANDBY;
			return;
		}
		InnoLogSystem::printLog("DXRenderingSystem: innoShader: " + l_shaderName + " Shader has been created.");

		D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
		unsigned int numElements;

		// Create the vertex input layout description.
		polygonLayout[0].SemanticName = "POSITION";
		polygonLayout[0].SemanticIndex = 0;
		polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		polygonLayout[0].InputSlot = 0;
		polygonLayout[0].AlignedByteOffset = 0;
		polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[0].InstanceDataStepRate = 0;

		polygonLayout[1].SemanticName = "TEXCOORD";
		polygonLayout[1].SemanticIndex = 0;
		polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		polygonLayout[1].InputSlot = 0;
		polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[1].InstanceDataStepRate = 0;

		polygonLayout[2].SemanticName = "NORMAL";
		polygonLayout[2].SemanticIndex = 0;
		polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		polygonLayout[2].InputSlot = 0;
		polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		polygonLayout[2].InstanceDataStepRate = 0;

		// Get a count of the elements in the layout.
		numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

		// Create the vertex input layout.
		result = m_device->CreateInputLayout(polygonLayout, numElements, shaderBuffer->GetBufferPointer(),
			shaderBuffer->GetBufferSize(), &DXFinalRenderPassSingletonComponent::getInstance().m_layout);
		if (FAILED(result))
		{
			InnoLogSystem::printLog("Error: DXRenderingSystem: can't create shader layout!");
			m_objectStatus = objectStatus::STANDBY;
			return;
		}
		break;

	case shaderType::GEOMETRY:
		break;

	case shaderType::FRAGMENT:
		// Compile the shader code.
		result = D3DCompileFromFile(l_shaderFilePath, NULL, NULL, l_shaderName.c_str(), "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
			&shaderBuffer, &errorMessage);
		if (FAILED(result))
		{
			// If the shader failed to compile it should have writen something to the error message.
			if (errorMessage)
			{
				OutputShaderErrorMessage(errorMessage, DXWindowSystemSingletonComponent::getInstance().m_hwnd, l_shaderName.c_str());
			}
			// If there was nothing in the error message then it simply could not find the shader file itself.
			else
			{
				MessageBox(DXWindowSystemSingletonComponent::getInstance().m_hwnd, l_shaderName.c_str(), "Missing Shader File", MB_OK);
				InnoLogSystem::printLog("Error: Shader creation failed: cannot find shader!");
			}

			return;
		}
		InnoLogSystem::printLog("DXRenderingSystem: innoShader: " + l_shaderName + " Shader has been compiled.");

		// Create the shader from the buffer.
		result = m_device->CreatePixelShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, &DXFinalRenderPassSingletonComponent::getInstance().m_pixelShader);
		if (FAILED(result))
		{
			InnoLogSystem::printLog("Error: DXRenderingSystem: can't create pixel shader!");
			m_objectStatus = objectStatus::STANDBY;
			return;
		}
		InnoLogSystem::printLog("DXRenderingSystem: innoShader: " + l_shaderName + " Shader has been created.");
		break;

	default:
		break;
	}

	shaderBuffer->Release();
	shaderBuffer = 0;
}

void DXRenderingSystem::OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename)
{
	char* compileErrors;
	unsigned long long bufferSize, i;
	std::stringstream errorSStream;

	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Write out the error message.
	for (i = 0; i<bufferSize; i++)
	{
		errorSStream << compileErrors[i];
	}

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	MessageBox(DXWindowSystemSingletonComponent::getInstance().m_hwnd, errorSStream.str().c_str(), shaderFilename.c_str(), MB_OK);
	InnoLogSystem::printLog("DXRenderingSystem: innoShader: " + shaderFilename + " compile error: " + errorSStream.str() + "\n -- --------------------------------------------------- -- ");
}

void DXRenderingSystem::initializeMesh(MeshDataComponent * rhs)
{
	auto l_ptr = reinterpret_cast<DXMeshDataComponent*>(rhs);

	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * (UINT)l_ptr->m_vertices.size();
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	//@TODO: InnoMath's vec4 is 32bit while XMFLOAT4 is 16bit
	vertexData.pSysMem = &l_ptr->m_vertices[0];
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &l_ptr->m_vertexBuffer);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create vbo!");
		return;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * (UINT)l_ptr->m_indices.size();
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = &l_ptr->m_indices[0];
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = m_device->CreateBuffer(&indexBufferDesc, &indexData, &l_ptr->m_indexBuffer);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create ibo!");
		return;
	}
	l_ptr->m_objectStatus = objectStatus::ALIVE;
}

void DXRenderingSystem::initializeTexture(TextureDataComponent * rhs)
{
	auto l_ptr = reinterpret_cast<DXTextureDataComponent*>(rhs);

	D3D11_TEXTURE2D_DESC textureDesc;
	HRESULT hResult;
	unsigned int rowPitch;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	// Setup the description of the texture.
	textureDesc.Height = l_ptr->m_textureHeight;
	textureDesc.Width = l_ptr->m_textureWidth;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	// Create the empty texture.
	hResult = m_device->CreateTexture2D(&textureDesc, NULL, &l_ptr->m_texture);
	if (FAILED(hResult))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create texture!");
		return;
	}

	rowPitch = (rhs->m_textureWidth * 4) * sizeof(unsigned char);
	m_deviceContext->UpdateSubresource(l_ptr->m_texture, 0, NULL, l_ptr->m_textureData[0], rowPitch, 0);

	// Setup the shader resource view description.
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	hResult = m_device->CreateShaderResourceView(l_ptr->m_texture, &srvDesc, &l_ptr->m_textureView);
	if (FAILED(hResult))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't create shader resource view for texture!");
		return;
	}

	// Generate mipmaps for this texture.
	m_deviceContext->GenerateMips(l_ptr->m_textureView);
}

void DXRenderingSystem::updateFinalBlendPass()
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(Vertex);
	offset = 0;
	auto l_mesh = InnoAssetSystem::getDefaultMesh(meshShapeType::CUBE);
	if (l_mesh)
	{
		if (l_mesh->m_objectStatus == objectStatus::ALIVE)
		{
			auto l_ptr = reinterpret_cast<DXMeshDataComponent*>(l_mesh);

			// Set the vertex buffer to active in the input assembler so it can be rendered.
			m_deviceContext->IASetVertexBuffers(0, 1, &l_ptr->m_vertexBuffer, &stride, &offset);

			// Set the index buffer to active in the input assembler so it can be rendered.
			m_deviceContext->IASetIndexBuffer(l_ptr->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

			// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
			m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Set the shader parameters that it will use for rendering.

			mat4 p = GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_projectionMatrix;
			mat4 r = InnoGameSystem::getTransformComponent(GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_parentEntity)->m_transform.getInvertGlobalRotationMatrix();
			mat4 t = InnoGameSystem::getTransformComponent(GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_parentEntity)->m_transform.getInvertGlobalTranslationMatrix();
			mat4 v = p * r * t;

			mat4 m = InnoMath::toTranslationMatrix(vec4(0.0, 0.0, -5.0, 1.0));
			auto mvp = v * m;

			updateShaderParameter(shaderType::VERTEX, DXFinalRenderPassSingletonComponent::getInstance().m_matrixBuffer, &mvp);

			// Set the vertex input layout.
			m_deviceContext->IASetInputLayout(DXFinalRenderPassSingletonComponent::getInstance().m_layout);

			// Set the vertex and pixel shaders that will be used to render this triangle.
			m_deviceContext->VSSetShader(DXFinalRenderPassSingletonComponent::getInstance().m_vertexShader, NULL, 0);
			m_deviceContext->PSSetShader(DXFinalRenderPassSingletonComponent::getInstance().m_pixelShader, NULL, 0);

			// Render the triangle.
			m_deviceContext->DrawIndexed((UINT)l_mesh->m_indices.size(), 0, 0);
		}
	}
}

void DXRenderingSystem::updateShaderParameter(shaderType shaderType, ID3D11Buffer * matrixBuffer, mat4* parameterValue)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	DirectX::XMMATRIX* dataPtr;
	unsigned int bufferNumber;

	//parameterValue = DirectX::XMMatrixTranspose(parameterValue);

	// Lock the constant buffer so it can be written to.
	result = m_deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		InnoLogSystem::printLog("Error: DXRenderingSystem: can't lock the shader matrix buffer!");
		m_objectStatus = objectStatus::STANDBY;
		return;
	}

	dataPtr = (DirectX::XMMATRIX*)mappedResource.pData;

	*dataPtr = *(DirectX::XMMATRIX*)parameterValue;

	// Unlock the constant buffer.
	m_deviceContext->Unmap(matrixBuffer, 0);

	bufferNumber = 0;

	switch (shaderType)
	{
	case shaderType::VERTEX:
		m_deviceContext->VSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);
		break;
	case shaderType::GEOMETRY:
		m_deviceContext->GSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);
		break;
	case shaderType::FRAGMENT:
		m_deviceContext->PSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);
		break;
	default:
		break;
	}
}

void DXRenderingSystem::beginScene(float r, float g, float b, float a)
{
	float color[4];

	// Setup the color to clear the buffer to.
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;

	// Clear the back buffer.
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);

	// Clear the depth buffer.
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DXRenderingSystem::endScene()
{
	// Present the back buffer to the screen since rendering is complete.
	if (m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		m_swapChain->Present(0, 0);
	}
}

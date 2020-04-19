#include "DX11RenderingServer.h"
#include "../../Component/DX11MeshDataComponent.h"
#include "../../Component/DX11TextureDataComponent.h"
#include "../../Component/DX11MaterialDataComponent.h"
#include "../../Component/DX11RenderPassDataComponent.h"
#include "../../Component/DX11ShaderProgramComponent.h"
#include "../../Component/DX11SamplerDataComponent.h"
#include "../../Component/DX11GPUBufferDataComponent.h"

#include "../../Component/WinWindowSystemComponent.h"

#include "DX11Helper.h"

using namespace DX11Helper;

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Interface/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

#include "../../Core/InnoLogger.h"
#include "../../Core/InnoMemory.h"
#include "../../Core/InnoRandomizer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace DX11RenderingServerNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	IObjectPool* m_MeshDataComponentPool = 0;
	IObjectPool* m_MaterialDataComponentPool = 0;
	IObjectPool* m_TextureDataComponentPool = 0;
	IObjectPool* m_RenderPassDataComponentPool = 0;
	IObjectPool* m_ResourcesBinderPool = 0;
	IObjectPool* m_PSOPool = 0;
	IObjectPool* m_ShaderProgramComponentPool = 0;
	IObjectPool* m_SamplerDataComponentPool = 0;
	IObjectPool* m_GPUBufferDataComponentPool = 0;

	std::unordered_set<MeshDataComponent*> m_initializedMeshes;
	std::unordered_set<TextureDataComponent*> m_initializedTextures;
	std::unordered_set<MaterialDataComponent*> m_initializedMaterials;

	TVec2<uint32_t> m_refreshRate = TVec2<uint32_t>(0, 1);

	int32_t m_videoCardMemory = 0;
	char m_videoCardDescription[128];

	IDXGIFactory* m_factory = 0;

	DXGI_ADAPTER_DESC m_adapterDesc = {};
	IDXGIAdapter* m_adapter = 0;
	IDXGIOutput* m_adapterOutput = 0;

	ID3D11Device5* m_device = 0;
	ID3D11DeviceContext4* m_deviceContext = 0;

	DXGI_SWAP_CHAIN_DESC m_swapChainDesc = {};
	IDXGISwapChain4* m_swapChain = 0;
	const uint32_t m_swapChainImageCount = 3;
	std::vector<ID3D11Texture2D*> m_swapChainTextures;

	ID3D10Blob* m_InputLayoutDummyShaderBuffer = 0;

	DX11RenderPassDataComponent* m_userPipelineOutput = 0;
	DX11RenderPassDataComponent* m_SwapChainRPDC = 0;
	DX11ShaderProgramComponent* m_SwapChainSPC = 0;
	DX11SamplerDataComponent* m_SwapChainSDC = 0;
}

using namespace DX11RenderingServerNS;

DX11ResourceBinder* addResourcesBinder()
{
	return InnoMemory::Spawn<DX11ResourceBinder>(m_ResourcesBinderPool);
}

DX11PipelineStateObject* addPSO()
{
	return InnoMemory::Spawn<DX11PipelineStateObject>(m_PSOPool);
}

bool DX11RenderingServer::Setup()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_MeshDataComponentPool = InnoMemory::CreateObjectPool<DX11MeshDataComponent>(l_renderingCapability.maxMeshes);
	m_TextureDataComponentPool = InnoMemory::CreateObjectPool<DX11TextureDataComponent>(l_renderingCapability.maxTextures);
	m_MaterialDataComponentPool = InnoMemory::CreateObjectPool<DX11MaterialDataComponent>(l_renderingCapability.maxMaterials);
	m_RenderPassDataComponentPool = InnoMemory::CreateObjectPool<DX11RenderPassDataComponent>(128);
	m_ResourcesBinderPool = InnoMemory::CreateObjectPool<DX11ResourceBinder>(16384);
	m_PSOPool = InnoMemory::CreateObjectPool<DX11PipelineStateObject>(128);
	m_ShaderProgramComponentPool = InnoMemory::CreateObjectPool<DX11ShaderProgramComponent>(256);
	m_SamplerDataComponentPool = InnoMemory::CreateObjectPool<DX11SamplerDataComponent>(256);
	m_GPUBufferDataComponentPool = InnoMemory::CreateObjectPool<DX11GPUBufferDataComponent>(256);

	HRESULT l_HResult;
	uint32_t l_numModes;
	uint64_t l_stringLength;

	// Create a DirectX graphics interface factory.
	l_HResult = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_factory);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create DXGI factory!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	l_HResult = m_factory->EnumAdapters(0, &m_adapter);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create video card adapter!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	l_HResult = m_adapter->EnumOutputs(0, &m_adapterOutput);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create monitor adapter!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	l_HResult = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, NULL);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	std::vector<DXGI_MODE_DESC> displayModeList(l_numModes);

	// Now fill the display mode list structures.
	l_HResult = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, &displayModeList[0]);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't fill the display mode list structures!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	for (uint32_t i = 0; i < l_numModes; i++)
	{
		if (displayModeList[i].Width == l_screenResolution.x
			&&
			displayModeList[i].Height == l_screenResolution.y
			)
		{
			m_refreshRate.x = displayModeList[i].RefreshRate.Numerator;
			m_refreshRate.y = displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Get the adapter (video card) description.
	l_HResult = m_adapter->GetDesc(&m_adapterDesc);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't get the video card adapter description!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int32_t)(m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	if (wcstombs_s(&l_stringLength, m_videoCardDescription, 128, m_adapterDesc.Description, 128) != 0)
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't convert the name of the video card to a character array!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	D3D_FEATURE_LEVEL featureLevel;

	// Initialize the swap chain description.
	ZeroMemory(&m_swapChainDesc, sizeof(m_swapChainDesc));

	m_swapChainDesc.BufferCount = m_swapChainImageCount;

	// Set the width and height of the back buffer.
	m_swapChainDesc.BufferDesc.Width = l_screenResolution.x;
	m_swapChainDesc.BufferDesc.Height = l_screenResolution.y;

	// Set regular 32-bit surface for the back buffer.
	m_swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.VSync)
	{
		m_swapChainDesc.BufferDesc.RefreshRate.Numerator = m_refreshRate.x;
		m_swapChainDesc.BufferDesc.RefreshRate.Denominator = m_refreshRate.y;
	}
	else
	{
		m_swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		m_swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	m_swapChainDesc.OutputWindow = WinWindowSystemComponent::get().m_hwnd;

	// Turn multisampling off.
	m_swapChainDesc.SampleDesc.Count = 1;
	m_swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	// @TODO: finish this feature
	m_swapChainDesc.Windowed = true;

	// Set the scan line ordering and scaling to unspecified.
	m_swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	// Don't set the advanced flags.
	m_swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_1;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	uint32_t creationFlags = 0;
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

	ID3D11Device* l_device;
	ID3D11DeviceContext* l_deviceContext;
	IDXGISwapChain* l_swapChain;

	l_HResult = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, &featureLevel, 1,
		D3D11_SDK_VERSION, &m_swapChainDesc, &l_swapChain, &l_device, NULL, &l_deviceContext);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create the swap chain/D3D device/D3D device context!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	m_device = reinterpret_cast<ID3D11Device5*>(l_device);
	m_deviceContext = reinterpret_cast<ID3D11DeviceContext4*>(l_deviceContext);
	m_swapChain = reinterpret_cast<IDXGISwapChain4*>(l_swapChain);

	m_swapChainTextures.resize(m_swapChainImageCount);

	// Get the pointer to the back buffer.

	for (size_t i = 0; i < m_swapChainImageCount; i++)
	{
		l_HResult = m_swapChain->GetBuffer((uint32_t)i, __uuidof(ID3D11Texture2D), (LPVOID*)&m_swapChainTextures[i]);

		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't get back buffer pointer!");
			m_ObjectStatus = ObjectStatus::Suspended;
			return false;
		}
	}

	// @TODO: Find a better solution
	LoadShaderFile(&m_InputLayoutDummyShaderBuffer, ShaderStage::Vertex, "common//dummyInputLayout.hlsl/");

	m_SwapChainRPDC = reinterpret_cast<DX11RenderPassDataComponent*>(AddRenderPassDataComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<DX11ShaderProgramComponent*>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSDC = reinterpret_cast<DX11SamplerDataComponent*>(AddSamplerDataComponent("SwapChain/"));

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "DX11RenderingServer setup finished.");

	return true;
}

bool DX11RenderingServer::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
		m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

		InitializeShaderProgramComponent(m_SwapChainSPC);

		InitializeSamplerDataComponent(m_SwapChainSDC);

		auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

		l_RenderPassDesc.m_RenderTargetCount = 1;

		m_SwapChainRPDC->m_RenderPassDesc = l_RenderPassDesc;
		m_SwapChainRPDC->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
		m_SwapChainRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

		m_SwapChainRPDC->m_ResourceBinderLayoutDescs.resize(2);
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceCount = 1;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Sampler;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;
		m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

		m_SwapChainRPDC->m_ShaderProgram = m_SwapChainSPC;

		ReserveRenderTargets(m_SwapChainRPDC, this);

		m_SwapChainRPDC->m_RenderTargetsResourceBinders.resize(m_SwapChainRPDC->m_RenderTargets.size());

		auto l_DX11TDC = reinterpret_cast<DX11TextureDataComponent*>(m_SwapChainRPDC->m_RenderTargets[0]);

		l_DX11TDC->m_ResourceHandle = m_swapChainTextures[0];
		l_DX11TDC->m_ObjectStatus = ObjectStatus::Activated;
		m_SwapChainRPDC->m_RenderTargetsResourceBinders[0] = addResourcesBinder();

		CreateViews(m_SwapChainRPDC, m_device);

		CreateResourcesBinder(m_SwapChainRPDC);

		m_SwapChainRPDC->m_PipelineStateObject = addPSO();

		CreateStateObjects(m_SwapChainRPDC, m_InputLayoutDummyShaderBuffer, m_device);

		m_SwapChainRPDC->m_ObjectStatus = ObjectStatus::Activated;
	}

	return true;
}

bool DX11RenderingServer::Terminate()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	m_swapChain->SetFullscreenState(false, NULL);

	m_deviceContext->Release();
	m_deviceContext = 0;

	m_device->Release();
	m_device = 0;

	m_swapChain->Release();
	m_swapChain = 0;

	m_adapterOutput->Release();
	m_adapterOutput = 0;

	m_adapter->Release();
	m_adapter = 0;

	m_factory->Release();
	m_factory = 0;

	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "DX11RenderingServer has been terminated.");

	return true;
}

ObjectStatus DX11RenderingServer::GetStatus()
{
	return m_ObjectStatus;
}

AddComponent(DX11, MeshData);
AddComponent(DX11, TextureData);
AddComponent(DX11, MaterialData);
AddComponent(DX11, RenderPassData);
AddComponent(DX11, ShaderProgram);
AddComponent(DX11, SamplerData);
AddComponent(DX11, GPUBufferData);

bool DX11RenderingServer::InitializeMeshDataComponent(MeshDataComponent* rhs)
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

	auto l_rhs = reinterpret_cast<DX11MeshDataComponent*>(rhs);

	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC l_vertexBufferDesc;
	ZeroMemory(&l_vertexBufferDesc, sizeof(l_vertexBufferDesc));
	l_vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	l_vertexBufferDesc.ByteWidth = sizeof(Vertex) * (uint32_t)l_rhs->m_vertices.size();
	l_vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	l_vertexBufferDesc.CPUAccessFlags = 0;
	l_vertexBufferDesc.MiscFlags = 0;
	l_vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA l_vertexSubresourceData;
	ZeroMemory(&l_vertexSubresourceData, sizeof(l_vertexSubresourceData));
	l_vertexSubresourceData.pSysMem = &l_rhs->m_vertices[0];
	l_vertexSubresourceData.SysMemPitch = 0;
	l_vertexSubresourceData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	HRESULT l_HResult;
	l_HResult = m_device->CreateBuffer(&l_vertexBufferDesc, &l_vertexSubresourceData, &l_rhs->m_vertexBuffer);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create Vertex Buffer!");
		return false;
	}
#ifdef  _DEBUG
	SetObjectName(l_rhs, l_rhs->m_vertexBuffer, "VB");
#endif //  _DEBUG

	InnoLogger::Log(LogLevel::Verbose, "DX11RenderingServer: Vertex Buffer: ", l_rhs->m_vertexBuffer, " is initialized.");

	// Set up the description of the static index buffer.
	D3D11_BUFFER_DESC l_indexBufferDesc;
	ZeroMemory(&l_indexBufferDesc, sizeof(l_indexBufferDesc));
	l_indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	l_indexBufferDesc.ByteWidth = (uint32_t)(l_rhs->m_indices.size() * sizeof(uint32_t));
	l_indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	l_indexBufferDesc.CPUAccessFlags = 0;
	l_indexBufferDesc.MiscFlags = 0;
	l_indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	D3D11_SUBRESOURCE_DATA l_indexSubresourceData;
	ZeroMemory(&l_indexSubresourceData, sizeof(l_indexSubresourceData));
	l_indexSubresourceData.pSysMem = &l_rhs->m_indices[0];
	l_indexSubresourceData.SysMemPitch = 0;
	l_indexSubresourceData.SysMemSlicePitch = 0;

	// Create the index buffer.
	l_HResult = m_device->CreateBuffer(&l_indexBufferDesc, &l_indexSubresourceData, &l_rhs->m_indexBuffer);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create Index Buffer!");
		return false;
	}
#ifdef  _DEBUG
	SetObjectName(l_rhs, l_rhs->m_indexBuffer, "IB");
#endif //  _DEBUG

	InnoLogger::Log(LogLevel::Verbose, "DX11RenderingServer: Index Buffer: ", l_rhs->m_indexBuffer, " is initialized.");

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool DX11RenderingServer::InitializeTextureDataComponent(TextureDataComponent* rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<DX11TextureDataComponent*>(rhs);

	l_rhs->m_DX11TextureDesc = GetDX11TextureDesc(l_rhs->m_TextureDesc);

	// Create the empty texture.
	HRESULT l_HResult;

	if (l_rhs->m_TextureDesc.Sampler == TextureSampler::Sampler1D)
	{
		auto l_desc = Get1DTextureDesc(l_rhs->m_DX11TextureDesc);
		l_HResult = m_device->CreateTexture1D(&l_desc, NULL, (ID3D11Texture1D**)&l_rhs->m_ResourceHandle);
	}
	else if (l_rhs->m_TextureDesc.Sampler == TextureSampler::Sampler2D)
	{
		auto l_desc = Get2DTextureDesc(l_rhs->m_DX11TextureDesc);
		l_HResult = m_device->CreateTexture2D(&l_desc, NULL, (ID3D11Texture2D**)&l_rhs->m_ResourceHandle);
	}
	else if (l_rhs->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
	{
		auto l_desc = Get3DTextureDesc(l_rhs->m_DX11TextureDesc);
		l_HResult = m_device->CreateTexture3D(&l_desc, NULL, (ID3D11Texture3D**)&l_rhs->m_ResourceHandle);
	}
	else
	{
		auto l_desc = Get2DTextureDesc(l_rhs->m_DX11TextureDesc);
		l_HResult = m_device->CreateTexture2D(&l_desc, NULL, (ID3D11Texture2D**)&l_rhs->m_ResourceHandle);
	}

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create Texture!");
		return false;
	}

	// Submit raw data to GPU memory
	if (l_rhs->m_TextureData)
	{
		uint32_t l_rowPitch = l_rhs->m_TextureDesc.Width * l_rhs->m_DX11TextureDesc.PixelDataSize;
		if (l_rhs->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
		{
			uint32_t l_depthPitch = l_rowPitch * l_rhs->m_TextureDesc.Height;
			m_deviceContext->UpdateSubresource(l_rhs->m_ResourceHandle, 0, NULL, l_rhs->m_TextureData, l_rowPitch, l_depthPitch);
		}
		else if (l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap)
		{
			for (uint32_t i = 0; i < 6; i++)
			{
				uint32_t l_subresource = D3D11CalcSubresource(0, i, l_rhs->m_DX11TextureDesc.MipLevels);
				void* l_rawData = (unsigned char*)l_rhs->m_TextureData + l_rowPitch * l_rhs->m_TextureDesc.Height * i;
				m_deviceContext->UpdateSubresource(l_rhs->m_ResourceHandle, l_subresource, NULL, l_rawData, l_rowPitch, 0);
			}
		}
		else
		{
			m_deviceContext->UpdateSubresource(l_rhs->m_ResourceHandle, 0, NULL, l_rhs->m_TextureData, l_rowPitch, 0);
		}
	}
#ifdef  _DEBUG
	SetObjectName(l_rhs, l_rhs->m_ResourceHandle, "Texture");
#endif //  _DEBUG

	InnoLogger::Log(LogLevel::Verbose, "DX11RenderingServer: Texture: ", l_rhs->m_ResourceHandle, " is initialized.");

	if (l_rhs->m_TextureDesc.CPUAccessibility == Accessibility::Immutable)
	{
		// Create SRV
		l_rhs->m_SRVDesc = GetSRVDesc(l_rhs->m_TextureDesc, l_rhs->m_DX11TextureDesc);

		l_HResult = m_device->CreateShaderResourceView(l_rhs->m_ResourceHandle, &l_rhs->m_SRVDesc, &l_rhs->m_SRV);
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create SRV for texture!");
			return false;
		}
#ifdef  _DEBUG
		SetObjectName(l_rhs, l_rhs->m_SRV, "SRV");
#endif //  _DEBUG

		InnoLogger::Log(LogLevel::Verbose, "DX11RenderingServer: SRV: ", l_rhs->m_SRV, " is initialized.");

		// Generate mipmaps for this texture.
		if (l_rhs->m_TextureDesc.UseMipMap)
		{
			m_deviceContext->GenerateMips(l_rhs->m_SRV);
		}

		// Create UAV
		if (l_rhs->m_TextureDesc.Usage == TextureUsage::RawImage)
		{
			l_rhs->m_UAVDesc = GetUAVDesc(l_rhs->m_TextureDesc, l_rhs->m_DX11TextureDesc);

			l_HResult = m_device->CreateUnorderedAccessView(l_rhs->m_ResourceHandle, &l_rhs->m_UAVDesc, &l_rhs->m_UAV);
			if (FAILED(l_HResult))
			{
				InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create UAV for texture!");
				return false;
			}
#ifdef  _DEBUG
			SetObjectName(l_rhs, l_rhs->m_UAV, "UAV");
#endif //  _DEBUG

			InnoLogger::Log(LogLevel::Verbose, "DX11RenderingServer: UAV: ", l_rhs->m_SRV, " is initialized.");
		}

		auto l_resourceBinder = addResourcesBinder();
		l_resourceBinder->m_GPUAccessibility = l_rhs->m_TextureDesc.GPUAccessibility;
		l_resourceBinder->m_SRV = l_rhs->m_SRV;
		l_resourceBinder->m_UAV = l_rhs->m_UAV;
		l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Image;
		l_rhs->m_ResourceBinder = l_resourceBinder;
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	return true;
}

bool DX11RenderingServer::InitializeMaterialDataComponent(MaterialDataComponent* rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<DX11MaterialDataComponent*>(rhs);

	auto l_defaultMaterial = g_pModuleManager->getRenderingFrontend()->getDefaultMaterialDataComponent();

	for (size_t i = 0; i < 8; i++)
	{
		auto l_texture = reinterpret_cast<DX11TextureDataComponent*>(l_rhs->m_TextureSlots[i].m_Texture);

		if (l_texture)
		{
			InitializeTextureDataComponent(l_texture);
			l_rhs->m_TextureSlots[i].m_Texture = l_texture;
			l_rhs->m_TextureSlots[i].m_Activate = true;
		}
		else
		{
			l_rhs->m_TextureSlots[i].m_Texture = l_defaultMaterial->m_TextureSlots[i].m_Texture;
		}
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(l_rhs);

	return true;
}

bool DX11RenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11RenderPassDataComponent*>(rhs);

	ReserveRenderTargets(l_rhs, this);

	CreateRenderTargets(l_rhs, this);

	CreateViews(l_rhs, m_device);

	l_rhs->m_RenderTargetsResourceBinders.resize(l_rhs->m_RenderPassDesc.m_RenderTargetCount);
	for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		l_rhs->m_RenderTargetsResourceBinders[i] = addResourcesBinder();
	}

	CreateResourcesBinder(l_rhs);

	l_rhs->m_PipelineStateObject = addPSO();

	CreateStateObjects(l_rhs, m_InputLayoutDummyShaderBuffer, m_device);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX11RenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11ShaderProgramComponent*>(rhs);

	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		ID3D10Blob* l_shaderFileBuffer = 0;
		LoadShaderFile(&l_shaderFileBuffer, ShaderStage::Vertex, l_rhs->m_ShaderFilePaths.m_VSPath);
		auto l_HResult = m_device->CreateVertexShader(l_shaderFileBuffer->GetBufferPointer(), l_shaderFileBuffer->GetBufferSize(), NULL, &l_rhs->m_VSHandle);
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create vertex shader!");
			return false;
		};
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		ID3D10Blob* l_shaderFileBuffer = 0;
		LoadShaderFile(&l_shaderFileBuffer, ShaderStage::Hull, l_rhs->m_ShaderFilePaths.m_HSPath);
		auto l_HResult = m_device->CreateHullShader(l_shaderFileBuffer->GetBufferPointer(), l_shaderFileBuffer->GetBufferSize(), NULL, &l_rhs->m_HSHandle);
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create hull shader!");
			return false;
		};
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		ID3D10Blob* l_shaderFileBuffer = 0;
		LoadShaderFile(&l_shaderFileBuffer, ShaderStage::Domain, l_rhs->m_ShaderFilePaths.m_DSPath);
		auto l_HResult = m_device->CreateDomainShader(l_shaderFileBuffer->GetBufferPointer(), l_shaderFileBuffer->GetBufferSize(), NULL, &l_rhs->m_DSHandle);
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create domain shader!");
			return false;
		};
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		ID3D10Blob* l_shaderFileBuffer = 0;
		LoadShaderFile(&l_shaderFileBuffer, ShaderStage::Geometry, l_rhs->m_ShaderFilePaths.m_GSPath);
		auto l_HResult = m_device->CreateGeometryShader(l_shaderFileBuffer->GetBufferPointer(), l_shaderFileBuffer->GetBufferSize(), NULL, &l_rhs->m_GSHandle);
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create geometry shader!");
			return false;
		};
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		ID3D10Blob* l_shaderFileBuffer = 0;
		LoadShaderFile(&l_shaderFileBuffer, ShaderStage::Pixel, l_rhs->m_ShaderFilePaths.m_PSPath);
		auto l_HResult = m_device->CreatePixelShader(l_shaderFileBuffer->GetBufferPointer(), l_shaderFileBuffer->GetBufferSize(), NULL, &l_rhs->m_FSHandle);
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create fragment shader!");
			return false;
		};
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		ID3D10Blob* l_shaderFileBuffer = 0;
		LoadShaderFile(&l_shaderFileBuffer, ShaderStage::Compute, l_rhs->m_ShaderFilePaths.m_CSPath);
		auto l_HResult = m_device->CreateComputeShader(l_shaderFileBuffer->GetBufferPointer(), l_shaderFileBuffer->GetBufferSize(), NULL, &l_rhs->m_CSHandle);
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create compute shader!");
			return false;
		};
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX11RenderingServer::InitializeSamplerDataComponent(SamplerDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11SamplerDataComponent*>(rhs);

	l_rhs->m_DX11SamplerDesc.Filter = GetFilterMode(l_rhs->m_SamplerDesc.m_MinFilterMethod, l_rhs->m_SamplerDesc.m_MagFilterMethod);
	l_rhs->m_DX11SamplerDesc.AddressU = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodU);
	l_rhs->m_DX11SamplerDesc.AddressV = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodV);
	l_rhs->m_DX11SamplerDesc.AddressW = GetWrapMode(l_rhs->m_SamplerDesc.m_WrapMethodW);
	l_rhs->m_DX11SamplerDesc.MipLODBias = 0.0f;
	l_rhs->m_DX11SamplerDesc.MaxAnisotropy = l_rhs->m_SamplerDesc.m_MaxAnisotropy;
	l_rhs->m_DX11SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	l_rhs->m_DX11SamplerDesc.BorderColor[0] = l_rhs->m_SamplerDesc.m_BorderColor[0];
	l_rhs->m_DX11SamplerDesc.BorderColor[1] = l_rhs->m_SamplerDesc.m_BorderColor[1];
	l_rhs->m_DX11SamplerDesc.BorderColor[2] = l_rhs->m_SamplerDesc.m_BorderColor[2];
	l_rhs->m_DX11SamplerDesc.BorderColor[3] = l_rhs->m_SamplerDesc.m_BorderColor[3];
	l_rhs->m_DX11SamplerDesc.MinLOD = l_rhs->m_SamplerDesc.m_MinLOD;
	l_rhs->m_DX11SamplerDesc.MaxLOD = l_rhs->m_SamplerDesc.m_MaxLOD;

	auto l_HResult = m_device->CreateSamplerState(&l_rhs->m_DX11SamplerDesc, &l_rhs->m_SamplerState);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create sampler state object for ", rhs->m_Name.c_str(), "!");
		return false;
	}

	auto l_resourceBinder = addResourcesBinder();
	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Sampler;
	l_resourceBinder->m_Sampler = l_rhs->m_SamplerState;

	l_rhs->m_ResourceBinder = l_resourceBinder;

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX11RenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11GPUBufferDataComponent*>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	auto l_resourceBinder = addResourcesBinder();
	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Buffer;
	l_resourceBinder->m_GPUAccessibility = l_rhs->m_GPUAccessibility;
	l_resourceBinder->m_ElementCount = l_rhs->m_ElementCount;
	l_resourceBinder->m_ElementSize = l_rhs->m_ElementSize;
	l_resourceBinder->m_TotalSize = l_rhs->m_TotalSize;

	auto l_isStructuredBuffer = (l_rhs->m_GPUAccessibility == Accessibility::ReadWrite);

	l_rhs->m_BufferDesc.ByteWidth = (uint32_t)(rhs->m_TotalSize);
	l_rhs->m_BufferDesc.Usage = l_isStructuredBuffer ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
	l_rhs->m_BufferDesc.BindFlags = l_isStructuredBuffer ? (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS) : D3D11_BIND_CONSTANT_BUFFER;
	l_rhs->m_BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	l_rhs->m_BufferDesc.MiscFlags = l_isStructuredBuffer ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : 0;
	l_rhs->m_BufferDesc.StructureByteStride = l_isStructuredBuffer ? (uint32_t)l_rhs->m_ElementSize : 0;

	HRESULT l_HResult;

	if (l_rhs->m_InitialData)
	{
		D3D11_SUBRESOURCE_DATA l_subresourceData;
		l_subresourceData.pSysMem = l_rhs->m_InitialData;
		l_subresourceData.SysMemPitch = 0;
		l_subresourceData.SysMemSlicePitch = 0;

		l_HResult = m_device->CreateBuffer(&l_rhs->m_BufferDesc, &l_subresourceData, &l_rhs->m_BufferPtr);
	}
	else
	{
		l_HResult = m_device->CreateBuffer(&l_rhs->m_BufferDesc, NULL, &l_rhs->m_BufferPtr);
	}
	l_resourceBinder->m_Buffer = l_rhs->m_BufferPtr;

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create Buffer object!");
		return false;
	}
#ifdef  _DEBUG
	if (l_isStructuredBuffer)
	{
		SetObjectName(l_rhs, l_rhs->m_BufferPtr, "SBuffer");
	}
	else
	{
		SetObjectName(l_rhs, l_rhs->m_BufferPtr, "CBuffer");
	}
#endif //  _DEBUG

	if (l_isStructuredBuffer)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC l_SRVDesc;
		l_SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		l_SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		l_SRVDesc.Buffer.FirstElement = 0;
		l_SRVDesc.Buffer.NumElements = (uint32_t)l_rhs->m_ElementCount;

		l_HResult = m_device->CreateShaderResourceView(l_rhs->m_BufferPtr, &l_SRVDesc, &l_rhs->m_SRV);

		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create SRV for Buffer object!");
			return false;
		}
#ifdef  _DEBUG
		SetObjectName(l_rhs, l_rhs->m_SRV, "SRV");
#endif //  _DEBUG
		l_resourceBinder->m_SRV = l_rhs->m_SRV;

		D3D11_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc;
		l_UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		l_UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		l_UAVDesc.Buffer.FirstElement = 0;
		l_UAVDesc.Buffer.NumElements = (uint32_t)l_rhs->m_ElementCount;
		l_UAVDesc.Buffer.Flags = 0;

		l_HResult = m_device->CreateUnorderedAccessView(l_rhs->m_BufferPtr, &l_UAVDesc, &l_rhs->m_UAV);

		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't create UAV for Buffer object!");
			return false;
		}
#ifdef  _DEBUG
		SetObjectName(l_rhs, l_rhs->m_UAV, "UAV");
#endif //  _DEBUG
		l_resourceBinder->m_UAV = l_rhs->m_UAV;
	}

	l_rhs->m_ResourceBinder = l_resourceBinder;

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DX11RenderingServer::DeleteMeshDataComponent(MeshDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11MeshDataComponent*>(rhs);

	l_rhs->m_vertexBuffer->Release();
	l_rhs->m_indexBuffer->Release();

	m_MeshDataComponentPool->Destroy(l_rhs);

	m_initializedMeshes.erase(l_rhs);

	return true;
}

bool DX11RenderingServer::DeleteTextureDataComponent(TextureDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11TextureDataComponent*>(rhs);

	l_rhs->m_ResourceHandle->Release();

	if (l_rhs->m_SRV)
	{
		l_rhs->m_SRV->Release();
	}

	if (l_rhs->m_UAV)
	{
		l_rhs->m_UAV->Release();
	}

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_TextureDataComponentPool->Destroy(l_rhs);

	m_initializedTextures.erase(l_rhs);

	return true;
}

bool DX11RenderingServer::DeleteMaterialDataComponent(MaterialDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11MaterialDataComponent*>(rhs);

	m_MaterialDataComponentPool->Destroy(l_rhs);

	m_initializedMaterials.erase(l_rhs);

	return true;
}

bool DX11RenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11RenderPassDataComponent*>(rhs);
	auto l_PSO = reinterpret_cast<DX11PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	l_PSO->m_RasterizerState->Release();
	if (l_PSO->m_BlendState)
	{
		l_PSO->m_BlendState->Release();
	}
	if (l_PSO->m_DepthStencilState)
	{
		l_PSO->m_DepthStencilState->Release();
	}
	l_PSO->m_InputLayout->Release();

	m_PSOPool->Destroy(l_PSO);

	if (l_rhs->m_DSV)
	{
		l_rhs->m_DSV->Release();
	}

	if (l_rhs->m_RenderPassDesc.m_UseDepthBuffer)
	{
		DeleteTextureDataComponent(l_rhs->m_DepthStencilRenderTarget);
	}

	for (size_t i = 0; i < l_rhs->m_RenderTargets.size(); i++)
	{
		if (l_rhs->m_RenderPassDesc.m_UseColorBuffer)
		{
			DeleteTextureDataComponent(l_rhs->m_RenderTargets[i]);
		}
		m_ResourcesBinderPool->Destroy(l_rhs->m_RenderTargetsResourceBinders[i]);
	}

	m_RenderPassDataComponentPool->Destroy(l_rhs);

	return true;
}

bool DX11RenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11ShaderProgramComponent*>(rhs);

	if (l_rhs->m_VSHandle)
	{
		l_rhs->m_VSHandle->Release();
	}
	if (l_rhs->m_HSHandle)
	{
		l_rhs->m_HSHandle->Release();
	}
	if (l_rhs->m_DSHandle)
	{
		l_rhs->m_DSHandle->Release();
	}
	if (l_rhs->m_GSHandle)
	{
		l_rhs->m_GSHandle->Release();
	}
	if (l_rhs->m_FSHandle)
	{
		l_rhs->m_FSHandle->Release();
	}
	if (l_rhs->m_CSHandle)
	{
		l_rhs->m_CSHandle->Release();
	}

	m_ShaderProgramComponentPool->Destroy(l_rhs);

	return true;
}

bool DX11RenderingServer::DeleteSamplerDataComponent(SamplerDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11SamplerDataComponent*>(rhs);

	l_rhs->m_SamplerState->Release();

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_SamplerDataComponentPool->Destroy(l_rhs);

	return true;
}

bool DX11RenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11GPUBufferDataComponent*>(rhs);

	l_rhs->m_BufferPtr->Release();

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_GPUBufferDataComponentPool->Destroy(l_rhs);

	return true;
}

bool DX11RenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_rhs = reinterpret_cast<DX11GPUBufferDataComponent*>(rhs);

	D3D11_MAP l_mapMethod;

	if (l_rhs->m_GPUAccessibility == Accessibility::ReadOnly)
	{
		l_mapMethod = D3D11_MAP_WRITE_DISCARD;
	}
	else
	{
		l_mapMethod = D3D11_MAP_WRITE;
	}

	D3D11_MAPPED_SUBRESOURCE l_MappedResource;

	auto l_HResult = m_deviceContext->Map(l_rhs->m_BufferPtr, 0, l_mapMethod, 0, &l_MappedResource);
	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't lock GPU Buffer!");
		return false;
	}

	auto l_dataPtr = (char*)l_MappedResource.pData;

	auto l_size = l_rhs->m_TotalSize;
	if (range != SIZE_MAX)
	{
		l_size = range * l_rhs->m_ElementSize;
	}

	std::memcpy(l_dataPtr + startOffset * l_rhs->m_ElementSize, GPUBufferValue, l_size);

	m_deviceContext->Unmap(l_rhs->m_BufferPtr, 0);

	return true;
}

bool DX11RenderingServer::CommandListBegin(RenderPassDataComponent* rhs, size_t frameIndex)
{
	return true;
}

bool DX11RenderingServer::BindRenderPassDataComponent(RenderPassDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11RenderPassDataComponent*>(rhs);
	auto l_PSO = reinterpret_cast<DX11PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	auto l_shaderProgram = reinterpret_cast<DX11ShaderProgramComponent*>(l_rhs->m_ShaderProgram);

	if (l_shaderProgram->m_VSHandle)
	{
		m_deviceContext->VSSetShader(l_shaderProgram->m_VSHandle, NULL, 0);
	}
	if (l_shaderProgram->m_HSHandle)
	{
		m_deviceContext->HSSetShader(l_shaderProgram->m_HSHandle, NULL, 0);
	}
	if (l_shaderProgram->m_DSHandle)
	{
		m_deviceContext->DSSetShader(l_shaderProgram->m_DSHandle, NULL, 0);
	}
	if (l_shaderProgram->m_GSHandle)
	{
		m_deviceContext->GSSetShader(l_shaderProgram->m_GSHandle, NULL, 0);
	}
	if (l_shaderProgram->m_FSHandle)
	{
		m_deviceContext->PSSetShader(l_shaderProgram->m_FSHandle, NULL, 0);
	}
	if (l_shaderProgram->m_CSHandle)
	{
		m_deviceContext->CSSetShader(l_shaderProgram->m_CSHandle, NULL, 0);
	}

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsage == RenderPassUsage::Graphics)
	{
		m_deviceContext->IASetInputLayout(l_PSO->m_InputLayout);
		m_deviceContext->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);

		m_deviceContext->RSSetViewports(1, &l_PSO->m_Viewport);
		m_deviceContext->RSSetState(l_PSO->m_RasterizerState);

		if (l_rhs->m_RenderPassDesc.m_RenderTargetCount)
		{
			if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.Usage != TextureUsage::RawImage)
			{
				if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
				{
					m_deviceContext->OMSetRenderTargets(1, &l_rhs->m_RTVs[l_rhs->m_CurrentFrame], l_rhs->m_DSV);
				}
				else
				{
					m_deviceContext->OMSetRenderTargets((uint32_t)l_rhs->m_RenderPassDesc.m_RenderTargetCount, &l_rhs->m_RTVs[0], l_rhs->m_DSV);
				}
			}
		}

		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			m_deviceContext->OMSetDepthStencilState(l_PSO->m_DepthStencilState, l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference);
		}
		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc.m_UseBlend)
		{
			m_deviceContext->OMSetBlendState(l_PSO->m_BlendState, NULL, 0xFFFFFFFF);
		}
	}

	return true;
}

bool DX11RenderingServer::CleanRenderTargets(RenderPassDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11RenderPassDataComponent*>(rhs);

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsage == RenderPassUsage::Graphics)
	{
		if (l_rhs->m_RenderPassDesc.m_RenderTargetCount)
		{
			if (l_rhs->m_RenderPassDesc.m_UseColorBuffer)
			{
				if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.Usage != TextureUsage::RawImage)
				{
					if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
					{
						m_deviceContext->ClearRenderTargetView(l_rhs->m_RTVs[l_rhs->m_CurrentFrame], l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor);
					}
					else
					{
						for (auto i : l_rhs->m_RTVs)
						{
							m_deviceContext->ClearRenderTargetView(i, l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor);
						}
					}
				}
				else
				{
					if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
					{
						auto l_RT = reinterpret_cast<DX11TextureDataComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame]);

						if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
						{
							m_deviceContext->ClearUnorderedAccessViewUint(l_RT->m_UAV, (UINT*)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor);
						}
						else
						{
							m_deviceContext->ClearUnorderedAccessViewFloat(l_RT->m_UAV, l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor);
						}
					}
					else
					{
						for (auto i : l_rhs->m_RenderTargets)
						{
							auto l_RT = reinterpret_cast<DX11TextureDataComponent*>(i);

							if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
							{
								m_deviceContext->ClearUnorderedAccessViewUint(l_RT->m_UAV, (UINT*)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor);
							}
							else
							{
								m_deviceContext->ClearUnorderedAccessViewFloat(l_RT->m_UAV, l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor);
							}
						}
					}
				}
			}

			if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
			{
				uint32_t l_flag = D3D11_CLEAR_DEPTH;
				if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite)
				{
					l_flag = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
				}

				m_deviceContext->ClearDepthStencilView(l_rhs->m_DSV, l_flag, 1.0f, 0x00);
			}
		}
	}

	return true;
}

bool BindSampler(ShaderStage shaderStage, uint32_t slot, ID3D11SamplerState* sampler)
{
	switch (shaderStage)
	{
	case ShaderStage::Vertex:
		m_deviceContext->VSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::Hull:
		m_deviceContext->HSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::Domain:
		m_deviceContext->DSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::Geometry:
		m_deviceContext->GSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::Pixel:
		m_deviceContext->PSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::Compute:
		m_deviceContext->CSSetSamplers(slot, 1, &sampler);
		break;
	default:
		break;
	}
	return true;
}

bool BindSRV(ShaderStage shaderStage, uint32_t slot, ID3D11ShaderResourceView* SRV)
{
	switch (shaderStage)
	{
	case ShaderStage::Vertex:
		m_deviceContext->VSSetShaderResources(slot, 1, &SRV);
		break;
	case ShaderStage::Hull:
		m_deviceContext->HSSetShaderResources(slot, 1, &SRV);
		break;
	case ShaderStage::Domain:
		m_deviceContext->DSSetShaderResources(slot, 1, &SRV);
		break;
	case ShaderStage::Geometry:
		m_deviceContext->GSSetShaderResources(slot, 1, &SRV);
		break;
	case ShaderStage::Pixel:
		m_deviceContext->PSSetShaderResources(slot, 1, &SRV);
		break;
	case ShaderStage::Compute:
		m_deviceContext->CSSetShaderResources(slot, 1, &SRV);
		break;
	default:
		break;
	}
	return true;
}

bool BindUAV(ShaderStage shaderStage, uint32_t slot, ID3D11UnorderedAccessView* UAV)
{
	if (shaderStage == ShaderStage::Compute)
	{
		m_deviceContext->CSSetUnorderedAccessViews((uint32_t)slot, 1, &UAV, nullptr);
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "DX11RenderingServer: Only allow Compute shader access UAV!");
		return false;
	}

	return true;
}

bool BindConstantBuffer(uint32_t slot, ID3D11Buffer* buffer, ShaderStage shaderStage)
{
	switch (shaderStage)
	{
	case ShaderStage::Vertex:
		m_deviceContext->VSSetConstantBuffers(slot, 1, &buffer);
		break;
	case ShaderStage::Hull:
		m_deviceContext->HSSetConstantBuffers(slot, 1, &buffer);
		break;
	case ShaderStage::Domain:
		m_deviceContext->DSSetConstantBuffers(slot, 1, &buffer);
		break;
	case ShaderStage::Geometry:
		m_deviceContext->GSSetConstantBuffers(slot, 1, &buffer);
		break;
	case ShaderStage::Pixel:
		m_deviceContext->PSSetConstantBuffers(slot, 1, &buffer);
		break;
	case ShaderStage::Compute:
		m_deviceContext->CSSetConstantBuffers(slot, 1, &buffer);
		break;
	default:
		break;
	}

	return true;
}

bool BindPartialConstantBuffer(uint32_t slot, ID3D11Buffer* buffer, ShaderStage shaderStage, size_t startOffset, size_t elementSize)
{
	auto l_constantCount = (uint32_t)elementSize / 16;
	auto l_firstConstant = (uint32_t)startOffset * l_constantCount;

	switch (shaderStage)
	{
	case ShaderStage::Vertex:
		m_deviceContext->VSSetConstantBuffers1(slot, 1, &buffer, &l_firstConstant, &l_constantCount);
		break;
	case ShaderStage::Hull:
		m_deviceContext->HSSetConstantBuffers1(slot, 1, &buffer, &l_firstConstant, &l_constantCount);
		break;
	case ShaderStage::Domain:
		m_deviceContext->DSSetConstantBuffers1(slot, 1, &buffer, &l_firstConstant, &l_constantCount);
		break;
	case ShaderStage::Geometry:
		m_deviceContext->GSSetConstantBuffers1(slot, 1, &buffer, &l_firstConstant, &l_constantCount);
		break;
	case ShaderStage::Pixel:
		m_deviceContext->PSSetConstantBuffers1(slot, 1, &buffer, &l_firstConstant, &l_constantCount);
		break;
	case ShaderStage::Compute:
		m_deviceContext->CSSetConstantBuffers1(slot, 1, &buffer, &l_firstConstant, &l_constantCount);
		break;
	default:
		break;
	}

	return true;
}

bool DX11RenderingServer::ActivateResourceBinder(RenderPassDataComponent* renderPass, ShaderStage shaderStage, IResourceBinder* binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	auto l_resourceBinder = reinterpret_cast<DX11ResourceBinder*>(binder);

	if (l_resourceBinder)
	{
		switch (l_resourceBinder->m_ResourceBinderType)
		{
		case ResourceBinderType::Sampler:
			BindSampler(shaderStage, (uint32_t)(localSlot), l_resourceBinder->m_Sampler);
			break;
		case ResourceBinderType::Image:
			if (accessibility != Accessibility::ReadOnly)
			{
				if (shaderStage == ShaderStage::Compute)
				{
					BindUAV(shaderStage, (uint32_t)(localSlot), l_resourceBinder->m_UAV);
				}
				else
				{
					auto l_renderPass = reinterpret_cast<DX11RenderPassDataComponent*>(renderPass);
					auto l_UAV = l_resourceBinder->m_UAV;
					const uint32_t l_initialCounts = -1;

					if (l_renderPass->m_RenderPassDesc.m_UseMultiFrames)
					{
						m_deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, l_renderPass->m_DSV, 0, 1, &l_UAV, &l_initialCounts);
					}
					else
					{
						auto l_RTCount = (uint32_t)l_renderPass->m_RenderPassDesc.m_RenderTargetCount;
						m_deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, l_renderPass->m_DSV, 0, l_RTCount, &l_UAV, &l_initialCounts);
					}
				}
			}
			else
			{
				BindSRV(shaderStage, (uint32_t)(localSlot), l_resourceBinder->m_SRV);
			}
			break;
		case ResourceBinderType::Buffer:
			if (l_resourceBinder->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				if (accessibility != Accessibility::ReadOnly)
				{
					InnoLogger::Log(LogLevel::Warning, "DX11RenderingServer: Not allow GPU write to Constant Buffer!");
				}
				if (elementCount != SIZE_MAX)
				{
					BindPartialConstantBuffer((uint32_t)localSlot, l_resourceBinder->m_Buffer, shaderStage, startOffset, l_resourceBinder->m_ElementSize);
				}
				else
				{
					BindConstantBuffer((uint32_t)localSlot, l_resourceBinder->m_Buffer, shaderStage);
				}
			}
			else
			{
				if (accessibility != Accessibility::ReadOnly)
				{
					BindUAV(shaderStage, (uint32_t)(localSlot), l_resourceBinder->m_UAV);
				}
				else
				{
					BindSRV(shaderStage, (uint32_t)(localSlot), l_resourceBinder->m_SRV);
				}
			}
			break;
		default:
			break;
		}
	}

	return true;
}

bool DX11RenderingServer::DeactivateResourceBinder(RenderPassDataComponent* renderPass, ShaderStage shaderStage, IResourceBinder* binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	auto l_resourceBinder = reinterpret_cast<DX11ResourceBinder*>(binder);

	if (l_resourceBinder)
	{
		switch (l_resourceBinder->m_ResourceBinderType)
		{
		case ResourceBinderType::Sampler:
			m_deviceContext->PSSetSamplers((uint32_t)localSlot, 1, 0);
			break;
		case ResourceBinderType::Image:
			if (accessibility != Accessibility::ReadOnly)
			{
				if (shaderStage == ShaderStage::Compute)
				{
					BindUAV(shaderStage, (uint32_t)(localSlot), 0);
				}
				else
				{
					m_deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 0, 0, NULL, NULL);
				}
			}
			else
			{
				BindSRV(shaderStage, (uint32_t)(localSlot), 0);
			}
			break;
		case ResourceBinderType::Buffer:
			if (l_resourceBinder->m_GPUAccessibility != Accessibility::ReadOnly)
			{
				if (accessibility != Accessibility::ReadOnly)
				{
					BindUAV(shaderStage, (uint32_t)(localSlot), 0);
				}
				else
				{
					BindSRV(shaderStage, (uint32_t)(localSlot), 0);
				}
			}
			break;
		default:
			break;
		}
	}

	return true;
}

bool DX11RenderingServer::DispatchDrawCall(RenderPassDataComponent* renderPass, MeshDataComponent* mesh, size_t instanceCount)
{
	auto l_rhs = reinterpret_cast<DX11MeshDataComponent*>(mesh);

	const uint32_t l_stride = sizeof(Vertex);
	const uint32_t l_offset = 0;

	m_deviceContext->IASetVertexBuffers(0, 1, &l_rhs->m_vertexBuffer, &l_stride, &l_offset);

	m_deviceContext->IASetIndexBuffer(l_rhs->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	m_deviceContext->DrawIndexedInstanced((uint32_t)l_rhs->m_indicesSize, (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX11RenderingServer::DispatchDrawCall(RenderPassDataComponent* renderPass, size_t instanceCount)
{
	m_deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

	m_deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

	m_deviceContext->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX11RenderingServer::CommandListEnd(RenderPassDataComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX11RenderPassDataComponent*>(rhs);
	auto l_PSO = reinterpret_cast<DX11PipelineStateObject*>(l_rhs->m_PipelineStateObject);

	auto l_shaderProgram = reinterpret_cast<DX11ShaderProgramComponent*>(l_rhs->m_ShaderProgram);

	if (l_shaderProgram->m_VSHandle)
	{
		m_deviceContext->VSSetShader(0, NULL, 0);
	}
	if (l_shaderProgram->m_HSHandle)
	{
		m_deviceContext->HSSetShader(0, NULL, 0);
	}
	if (l_shaderProgram->m_DSHandle)
	{
		m_deviceContext->DSSetShader(0, NULL, 0);
	}
	if (l_shaderProgram->m_GSHandle)
	{
		m_deviceContext->GSSetShader(0, NULL, 0);
	}
	if (l_shaderProgram->m_FSHandle)
	{
		m_deviceContext->PSSetShader(0, NULL, 0);
	}
	if (l_shaderProgram->m_CSHandle)
	{
		m_deviceContext->CSSetShader(0, NULL, 0);
	}

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsage == RenderPassUsage::Graphics)
	{
		m_deviceContext->IASetInputLayout(NULL);
		m_deviceContext->RSSetState(NULL);

		if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.Usage != TextureUsage::RawImage)
		{
			m_deviceContext->OMSetRenderTargets(0, NULL, NULL);
		}

		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			m_deviceContext->OMSetDepthStencilState(NULL, 0x00);
		}
		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc.m_UseBlend)
		{
			m_deviceContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
		}
	}

	return true;
}

bool DX11RenderingServer::ExecuteCommandList(RenderPassDataComponent* rhs)
{
	return true;
}

bool DX11RenderingServer::WaitForFrame(RenderPassDataComponent* rhs)
{
	return true;
}

bool DX11RenderingServer::SetUserPipelineOutput(RenderPassDataComponent* rhs)
{
	m_userPipelineOutput = reinterpret_cast<DX11RenderPassDataComponent*>(rhs);

	return true;
}

bool DX11RenderingServer::Present()
{
	CommandListBegin(m_SwapChainRPDC, m_SwapChainRPDC->m_CurrentFrame);

	BindRenderPassDataComponent(m_SwapChainRPDC);

	CleanRenderTargets(m_SwapChainRPDC);

	ActivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_SwapChainSDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	ActivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_userPipelineOutput->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Square);

	DispatchDrawCall(m_SwapChainRPDC, l_mesh, 1);

	DeactivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_userPipelineOutput->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	CommandListEnd(m_SwapChainRPDC);

	ExecuteCommandList(m_SwapChainRPDC);

	WaitForFrame(m_SwapChainRPDC);

	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.VSync)
	{
		m_swapChain->Present(1, 0);
	}
	else
	{
		m_swapChain->Present(0, 0);
	}

	return true;
}

bool DX11RenderingServer::DispatchCompute(RenderPassDataComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	m_deviceContext->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

bool DX11RenderingServer::CopyDepthStencilBuffer(RenderPassDataComponent* src, RenderPassDataComponent* dest)
{
	auto l_src = reinterpret_cast<DX11TextureDataComponent*>(src->m_DepthStencilRenderTarget);
	auto l_dest = reinterpret_cast<DX11TextureDataComponent*>(dest->m_DepthStencilRenderTarget);

	m_deviceContext->CopyResource(l_dest->m_ResourceHandle, l_src->m_ResourceHandle);

	return true;
}

bool DX11RenderingServer::CopyColorBuffer(RenderPassDataComponent* src, size_t srcIndex, RenderPassDataComponent* dest, size_t destIndex)
{
	auto l_src = reinterpret_cast<DX11TextureDataComponent*>(src->m_RenderTargets[srcIndex]);
	auto l_dest = reinterpret_cast<DX11TextureDataComponent*>(dest->m_RenderTargets[srcIndex]);

	m_deviceContext->CopyResource(l_dest->m_ResourceHandle, l_src->m_ResourceHandle);

	return true;
}

Vec4 DX11RenderingServer::ReadRenderTargetSample(RenderPassDataComponent* rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> DX11RenderingServer::ReadTextureBackToCPU(RenderPassDataComponent* canvas, TextureDataComponent* TDC)
{
	// @TODO: Support different pixel data type
	auto l_srcTDC = reinterpret_cast<DX11TextureDataComponent*>(TDC);

	std::vector<uint32_t> l_DSResult;
	std::vector<Vec4> l_result;

	auto l_destTDC = reinterpret_cast<DX11TextureDataComponent*>(AddTextureDataComponent("ReadBackTemp/"));
	l_destTDC->m_TextureDesc = TDC->m_TextureDesc;
	l_destTDC->m_TextureDesc.CPUAccessibility = Accessibility::ReadOnly;

	InitializeTextureDataComponent(l_destTDC);

	m_deviceContext->CopyResource(l_destTDC->m_ResourceHandle, l_srcTDC->m_ResourceHandle);

	D3D11_MAPPED_SUBRESOURCE l_mappedResource;
	auto l_HResult = m_deviceContext->Map(l_destTDC->m_ResourceHandle, 0, D3D11_MAP_READ, 0, &l_mappedResource);

	if (FAILED(l_HResult))
	{
		InnoLogger::Log(LogLevel::Error, "DX11RenderingServer: Can't map texture for CPU to read!");
	}
	else
	{
		size_t l_sampleCount = 0;
		size_t l_sliceCount = 1;

		switch (l_srcTDC->m_TextureDesc.Sampler)
		{
		case TextureSampler::Sampler1D:
			l_sampleCount = l_srcTDC->m_TextureDesc.Width;
			break;
		case TextureSampler::Sampler2D:
			l_sampleCount = l_srcTDC->m_TextureDesc.Width * l_srcTDC->m_TextureDesc.Height;
			break;
		case TextureSampler::Sampler3D:
			l_sampleCount = l_srcTDC->m_TextureDesc.Width * l_srcTDC->m_TextureDesc.Height * l_srcTDC->m_TextureDesc.DepthOrArraySize;
			l_sliceCount = l_srcTDC->m_TextureDesc.DepthOrArraySize;
			break;
		case TextureSampler::Sampler1DArray:
			l_sampleCount = l_srcTDC->m_TextureDesc.Width * l_srcTDC->m_TextureDesc.DepthOrArraySize;
			l_sliceCount = l_srcTDC->m_TextureDesc.DepthOrArraySize;
			break;
		case TextureSampler::Sampler2DArray:
			l_sampleCount = l_srcTDC->m_TextureDesc.Width * l_srcTDC->m_TextureDesc.Height * l_srcTDC->m_TextureDesc.DepthOrArraySize;
			l_sliceCount = l_srcTDC->m_TextureDesc.DepthOrArraySize;
			break;
		case TextureSampler::SamplerCubemap:
			l_sampleCount = l_srcTDC->m_TextureDesc.Width * l_srcTDC->m_TextureDesc.Height * 6;
			l_sliceCount = 6;
			break;
		default:
			break;
		}
		l_result.resize(l_sampleCount);
		l_DSResult.resize(l_sampleCount);

		if (l_mappedResource.DepthPitch)
		{
			if (l_srcTDC->m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::Depth)
			{
				std::memcpy(l_DSResult.data(), l_mappedResource.pData, l_mappedResource.DepthPitch * l_sliceCount);
				for (size_t i = 0; i < l_sampleCount; i++)
				{
					auto l_depth = float(l_DSResult[i]);
					l_result[i].x = l_depth;
				}
			}
			else if (l_srcTDC->m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
			{
				std::memcpy(l_DSResult.data(), l_mappedResource.pData, l_mappedResource.DepthPitch * l_sliceCount);
				for (size_t i = 0; i < l_sampleCount; i++)
				{
					auto l_depth = l_DSResult[i] & 0x00FFFFFF;
					auto l_stencil = (l_DSResult[i] & 0xFF000000) >> 24;
					l_result[i].x = float(l_depth) / float(0x00FFFFFF);
					l_result[i].y = float(l_stencil);
				}
			}
			else
			{
				std::memcpy(l_result.data(), l_mappedResource.pData, l_mappedResource.DepthPitch * l_sliceCount);
			}
		}
		else
		{
			std::memcpy(l_result.data(), l_mappedResource.pData, l_mappedResource.RowPitch * l_sliceCount);
		}
	}

	m_deviceContext->Unmap(l_destTDC->m_ResourceHandle, 0);

	DeleteTextureDataComponent(l_destTDC);

	return l_result;
}

bool DX11RenderingServer::Resize()
{
	return true;
}

bool DX11RenderingServer::BeginCapture()
{
	return false;
}

bool DX11RenderingServer::EndCapture()
{
	return false;
}

void* DX11RenderingServer::GetDevice()
{
	return m_device;
}

void* DX11RenderingServer::GetDeviceContext()
{
	return m_deviceContext;
}
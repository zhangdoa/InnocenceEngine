#include "DX11RenderingSystemUtilities.h"

#include "../../component/WinWindowSystemComponent.h"
#include "../../component/DX11RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DX11RenderingSystemNS
{
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);
	ID3D10Blob* loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath);
	bool createCBuffer(DX11CBuffer& arg);
	bool initializeVertexShader(DX11ShaderProgramComponent* rhs, const std::wstring& VSShaderPath);
	bool createVertexShader(ID3D10Blob* shaderBuffer, ID3D11VertexShader** vertexShader);
	bool createInputLayout(ID3D10Blob* shaderBuffer, ID3D11InputLayout** inputLayout);
	bool initializePixelShader(DX11ShaderProgramComponent* rhs, const std::wstring& PSShaderPath);
	bool createPixelShader(ID3D10Blob* shaderBuffer, ID3D11PixelShader** pixelShader);

	bool initializeDX11MeshDataComponent(DX11MeshDataComponent * rhs, const std::vector<Vertex>& vertices, const std::vector<Index>& indices);
	bool initializeDX11TextureDataComponent(DX11TextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData);

	std::unordered_map<EntityID, DX11MeshDataComponent*> m_initializedDXMDC;
	std::unordered_map<EntityID, DX11TextureDataComponent*> m_initializedDXTDC;

	std::unordered_map<EntityID, DX11MeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, DX11TextureDataComponent*> m_textureMap;

	void* m_DX11MeshDataComponentPool;
	void* m_DX11TextureDataComponentPool;
	void* m_DX11RenderPassComponentPool;
	void* m_DX11ShaderProgramComponentPool;

	const std::wstring m_shaderRelativePath = L"..//res//shaders//";
}

bool DX11RenderingSystemNS::initializeComponentPool()
{
	m_DX11MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX11MeshDataComponent), 32768);
	m_DX11TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX11TextureDataComponent), 32768);
	m_DX11RenderPassComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX11RenderPassComponent), 128);
	m_DX11ShaderProgramComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX11ShaderProgramComponent), 256);

	return true;
}

ID3D10Blob* DX11RenderingSystemNS::loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath)
{
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

	result = D3DCompileFromFile((m_shaderRelativePath + shaderFilePath).c_str(), NULL, NULL, l_shaderName.c_str(), l_shaderTypeName.c_str(), D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&l_shaderBuffer, &l_errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (l_errorMessage)
		{
			OutputShaderErrorMessage(l_errorMessage, WinWindowSystemComponent::get().m_hwnd, l_shaderName.c_str());
		}
		// If there was nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(WinWindowSystemComponent::get().m_hwnd, l_shaderName.c_str(), "Missing Shader File", MB_OK);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "Shader creation failed: cannot find shader!");
		}

		return nullptr;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingSystem: innoShader: " + l_shaderName + " Shader has been compiled.");
	return l_shaderBuffer;
}

bool DX11RenderingSystemNS::createCBuffer(DX11CBuffer& arg)
{
	if (arg.m_CBufferDesc.ByteWidth > 0)
	{
		// Create the constant buffer pointer
		auto result = DX11RenderingSystemComponent::get().m_device->CreateBuffer(&arg.m_CBufferDesc, NULL, &arg.m_CBufferPtr);

		if (FAILED(result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create constant buffer pointer!");
			return false;
		}

		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: constant buffer byte width is 0!");
		return false;
	}
}

bool DX11RenderingSystemNS::initializeVertexShader(DX11ShaderProgramComponent* rhs, const std::wstring& VSShaderPath)
{
	// Compile the shader code.
	auto l_shaderBuffer = loadShaderBuffer(ShaderType::VERTEX, VSShaderPath);

	if (!createVertexShader(l_shaderBuffer, &rhs->m_vertexShader))
	{
		return false;
	}

	if (!createInputLayout(l_shaderBuffer, &rhs->m_inputLayout))
	{
		return false;
	}

	l_shaderBuffer->Release();

	return true;
}

bool DX11RenderingSystemNS::createVertexShader(ID3D10Blob* shaderBuffer, ID3D11VertexShader** vertexShader)
{
	auto result = DX11RenderingSystemComponent::get().m_device->CreateVertexShader(
		shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
		NULL,
		vertexShader);

	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create vertex shader!");
		return false;
	}

	return true;
}

bool DX11RenderingSystemNS::createInputLayout(ID3D10Blob* shaderBuffer, ID3D11InputLayout** inputLayout)
{
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
	auto result = DX11RenderingSystemComponent::get().m_device->CreateInputLayout(
		l_polygonLayout, l_numElements, shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), inputLayout);

	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create vertex shader layout!");
		return false;
	}

	return true;
}

bool DX11RenderingSystemNS::initializePixelShader(DX11ShaderProgramComponent* rhs, const std::wstring& PSShaderPath)
{
	// Compile the shader code.
	auto l_shaderBuffer = loadShaderBuffer(ShaderType::FRAGMENT, PSShaderPath);

	if (!createPixelShader(l_shaderBuffer, &rhs->m_pixelShader))
	{
		return false;
	}

	// Create the texture sampler state.
	auto result = DX11RenderingSystemComponent::get().m_device->CreateSamplerState(
		&rhs->m_samplerDesc,
		&rhs->m_samplerState);

	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create texture sampler state!");
		return false;
	}

	l_shaderBuffer->Release();

	return true;
}

bool DX11RenderingSystemNS::createPixelShader(ID3D10Blob* shaderBuffer, ID3D11PixelShader** pixelShader)
{
	auto result = DX11RenderingSystemComponent::get().m_device->CreatePixelShader(
		shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
		NULL,
		pixelShader);

	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create pixel shader!");
		return false;
	}

	return true;
}

DX11ShaderProgramComponent* DX11RenderingSystemNS::addDX11ShaderProgramComponent(EntityID rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_DX11ShaderProgramComponentPool, sizeof(DX11ShaderProgramComponent));
	auto l_DXSPC = new(l_rawPtr)DX11ShaderProgramComponent();
	l_DXSPC->m_parentEntity = rhs;
	return l_DXSPC;
}

bool DX11RenderingSystemNS::initializeDX11ShaderProgramComponent(DX11ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths)
{
	bool l_result = true;
	if (shaderFilePaths.m_VSPath != "")
	{
		l_result = l_result && initializeVertexShader(rhs, std::wstring(shaderFilePaths.m_VSPath.begin(), shaderFilePaths.m_VSPath.end()));
	}
	if (shaderFilePaths.m_FSPath != "")
	{
		l_result = l_result && initializePixelShader(rhs, std::wstring(shaderFilePaths.m_FSPath.begin(), shaderFilePaths.m_FSPath.end()));
	}
	for (auto& i : rhs->m_VSCBuffers)
	{
		l_result = l_result && createCBuffer(i);
	}
	for (auto& i : rhs->m_PSCBuffers)
	{
		l_result = l_result && createCBuffer(i);
	}

	return l_result;
}

void DX11RenderingSystemNS::OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename)
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

	MessageBox(WinWindowSystemComponent::get().m_hwnd, errorSStream.str().c_str(), shaderFilename.c_str(), MB_OK);
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: innoShader: " + shaderFilename + " compile error: " + errorSStream.str() + "\n -- --------------------------------------------------- -- ");
}

bool DX11RenderingSystemNS::activateDX11ShaderProgramComponent(DX11ShaderProgramComponent * rhs)
{
	if (rhs->m_vertexShader)
	{
		DX11RenderingSystemComponent::get().m_deviceContext->VSSetShader(
			rhs->m_vertexShader,
			NULL,
			0);

		DX11RenderingSystemComponent::get().m_deviceContext->IASetInputLayout(rhs->m_inputLayout);
	}
	if (rhs->m_pixelShader)
	{
		DX11RenderingSystemComponent::get().m_deviceContext->PSSetShader(
			rhs->m_pixelShader,
			NULL,
			0);

		DX11RenderingSystemComponent::get().m_deviceContext->PSSetSamplers(0, 1, &rhs->m_samplerState);
	}

	return true;
}

DX11RenderPassComponent* DX11RenderingSystemNS::addDX11RenderPassComponent(unsigned int RTNum, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc, TextureDataDesc RTDesc)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_DX11RenderPassComponentPool, sizeof(DX11RenderPassComponent));
	auto l_DXRPC = new(l_rawPtr)DX11RenderPassComponent();

	HRESULT result;

	// create TDC
	l_DXRPC->m_TDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = g_pCoreSystem->getAssetSystem()->addTextureDataComponent();

		l_TDC->m_textureDataDesc.textureSamplerType = RTDesc.textureSamplerType;
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
		auto l_DXTDC = generateDX11TextureDataComponent(l_TDC);

		l_DXRPC->m_DXTDCs.emplace_back(l_DXTDC);
	}

	// Create the render target views.
	l_DXRPC->m_renderTargetViews.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		l_DXRPC->m_renderTargetViews.emplace_back();
		result = DX11RenderingSystemComponent::get().m_device->CreateRenderTargetView(
			l_DXRPC->m_DXTDCs[i]->m_texture,
			&renderTargetViewDesc,
			&l_DXRPC->m_renderTargetViews[i]);
		if (FAILED(result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create render target view!");
			return nullptr;
		}
	}

	// Initialize the description of the depth buffer.
	ZeroMemory(&l_DXRPC->m_depthBufferDesc,
		sizeof(l_DXRPC->m_depthBufferDesc));

	// Set up the description of the depth buffer.
	l_DXRPC->m_depthBufferDesc.Width = RTDesc.textureWidth;
	l_DXRPC->m_depthBufferDesc.Height = RTDesc.textureHeight;
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
	result = DX11RenderingSystemComponent::get().m_device->CreateTexture2D(
		&l_DXRPC->m_depthBufferDesc,
		NULL,
		&l_DXRPC->m_depthStencilBuffer);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the texture for the depth buffer!");
		return nullptr;
	}

	// Initailze the depth stencil view description.
	ZeroMemory(&l_DXRPC->m_depthStencilViewDesc,
		sizeof(l_DXRPC->m_depthStencilViewDesc));

	// Set up the depth stencil view description.
	l_DXRPC->m_depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	l_DXRPC->m_depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	l_DXRPC->m_depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = DX11RenderingSystemComponent::get().m_device->CreateDepthStencilView(
		l_DXRPC->m_depthStencilBuffer,
		&l_DXRPC->m_depthStencilViewDesc,
		&l_DXRPC->m_depthStencilView);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the depth stencil view!");
		return nullptr;
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

DX11MeshDataComponent* DX11RenderingSystemNS::generateDX11MeshDataComponent(MeshDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getDX11MeshDataComponent(rhs->m_parentEntity);
	}
	else
	{
		auto l_ptr = addDX11MeshDataComponent(rhs->m_parentEntity);

		initializeDX11MeshDataComponent(l_ptr, rhs->m_vertices, rhs->m_indices);

		rhs->m_objectStatus = ObjectStatus::ALIVE;

		return l_ptr;
	}
}

bool DX11RenderingSystemNS::initializeDX11MeshDataComponent(DX11MeshDataComponent * rhs, const std::vector<Vertex>& vertices, const std::vector<Index>& indices)
{
	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * (UINT)vertices.size();
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA vertexData;
	ZeroMemory(&vertexData, sizeof(vertexData));
	vertexData.pSysMem = &vertices[0];
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	HRESULT result;
	result = DX11RenderingSystemComponent::get().m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &rhs->m_vertexBuffer);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create VBO!");
		return false;
	}

	// Set up the description of the static index buffer.
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = (UINT)(indices.size() * sizeof(unsigned int));
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	D3D11_SUBRESOURCE_DATA indexData;
	ZeroMemory(&indexData, sizeof(indexData));
	indexData.pSysMem = &indices[0];
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = DX11RenderingSystemComponent::get().m_device->CreateBuffer(&indexBufferDesc, &indexData, &rhs->m_indexBuffer);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create IBO!");
		return false;
	}
	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedDXMDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingSystem: VBO " + InnoUtility::pointerToString(rhs->m_vertexBuffer) + " is initialized.");

	return true;
}

DX11TextureDataComponent* DX11RenderingSystemNS::generateDX11TextureDataComponent(TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getDX11TextureDataComponent(rhs->m_parentEntity);
	}
	else
	{
		if (rhs->m_textureDataDesc.textureUsageType == TextureUsageType::INVISIBLE)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: TextureUsageType is TextureUsageType::INVISIBLE!");
			return nullptr;
		}
		else
		{
			auto l_ptr = addDX11TextureDataComponent(rhs->m_parentEntity);

			initializeDX11TextureDataComponent(l_ptr, rhs->m_textureDataDesc, rhs->m_textureData);

			rhs->m_objectStatus = ObjectStatus::ALIVE;

			return l_ptr;
		}
	}
}

bool DX11RenderingSystemNS::initializeDX11TextureDataComponent(DX11TextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData)
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
	hResult = DX11RenderingSystemComponent::get().m_device->CreateTexture2D(&D3DTextureDesc, NULL, &rhs->m_texture);
	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create texture!");
		return false;
	}

	if (textureDataDesc.textureUsageType != TextureUsageType::RENDER_TARGET)
	{
		unsigned int rowPitch;
		rowPitch = (textureDataDesc.textureWidth * ((unsigned int)textureDataDesc.texturePixelDataFormat + 1)) * sizeof(unsigned char);
		DX11RenderingSystemComponent::get().m_deviceContext->UpdateSubresource(rhs->m_texture, 0, NULL, textureData[0], rowPitch, 0);
	}

	// Create the shader resource view for the texture.
	hResult = DX11RenderingSystemComponent::get().m_device->CreateShaderResourceView(rhs->m_texture, &srvDesc, &rhs->m_SRV);
	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create shader resource view for texture!");
		return false;
	}

	// Generate mipmaps for this texture.
	if (textureDataDesc.textureMagFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		DX11RenderingSystemComponent::get().m_deviceContext->GenerateMips(rhs->m_SRV);
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedDXTDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingSystem: SRV " + InnoUtility::pointerToString(rhs->m_SRV) + " is initialized.");

	return true;
}

DX11MeshDataComponent* DX11RenderingSystemNS::addDX11MeshDataComponent(EntityID rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_DX11MeshDataComponentPool, sizeof(DX11MeshDataComponent));
	auto l_DXMDC = new(l_rawPtr)DX11MeshDataComponent();
	l_DXMDC->m_parentEntity = rhs;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, DX11MeshDataComponent*>(rhs, l_DXMDC));
	return l_DXMDC;
}

DX11TextureDataComponent* DX11RenderingSystemNS::addDX11TextureDataComponent(EntityID rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_DX11TextureDataComponentPool, sizeof(DX11TextureDataComponent));
	auto l_DXTDC = new(l_rawPtr)DX11TextureDataComponent();
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, DX11TextureDataComponent*>(rhs, l_DXTDC));
	return l_DXTDC;
}

DX11MeshDataComponent * DX11RenderingSystemNS::getDX11MeshDataComponent(EntityID rhs)
{
	auto result = m_meshMap.find(rhs);
	if (result != m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

DX11TextureDataComponent * DX11RenderingSystemNS::getDX11TextureDataComponent(EntityID rhs)
{
	auto result = m_textureMap.find(rhs);
	if (result != m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

void DX11RenderingSystemNS::drawMesh(EntityID rhs)
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(rhs);
	if (l_MDC)
	{
		drawMesh(l_MDC);
	}
}

void DX11RenderingSystemNS::drawMesh(MeshDataComponent * MDC)
{
	auto l_DXMDC = DX11RenderingSystemNS::getDX11MeshDataComponent(MDC->m_parentEntity);
	if (l_DXMDC)
	{
		if (MDC->m_objectStatus == ObjectStatus::ALIVE && l_DXMDC->m_objectStatus == ObjectStatus::ALIVE)
		{
			drawMesh(MDC->m_indicesSize, l_DXMDC);
		}
	}
}

void DX11RenderingSystemNS::drawMesh(size_t indicesSize, DX11MeshDataComponent * DXMDC)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(Vertex);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	DX11RenderingSystemComponent::get().m_deviceContext->IASetVertexBuffers(0, 1, &DXMDC->m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	DX11RenderingSystemComponent::get().m_deviceContext->IASetIndexBuffer(DXMDC->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Render the triangle.
	DX11RenderingSystemComponent::get().m_deviceContext->DrawIndexed((UINT)indicesSize, 0, 0);
}

void DX11RenderingSystemNS::updateShaderParameter(ShaderType shaderType, unsigned int startSlot, const std::vector<DX11CBuffer>& DX11CBuffers, void* parameterValue)
{
	auto l_DX11CBuffer = DX11CBuffers[startSlot];

	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Lock the constant buffer so it can be written to.
	result = DX11RenderingSystemComponent::get().m_deviceContext->Map(l_DX11CBuffer.m_CBufferPtr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't lock the shader buffer!");
		return;
	}

	auto dataPtr = mappedResource.pData;
	std::memcpy(dataPtr, parameterValue, l_DX11CBuffer.m_CBufferDesc.ByteWidth);

	// Unlock the constant buffer.
	DX11RenderingSystemComponent::get().m_deviceContext->Unmap(l_DX11CBuffer.m_CBufferPtr, 0);

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DX11RenderingSystemComponent::get().m_deviceContext->VSSetConstantBuffers(startSlot, 1, &l_DX11CBuffer.m_CBufferPtr);
		break;
	case ShaderType::GEOMETRY:
		DX11RenderingSystemComponent::get().m_deviceContext->GSSetConstantBuffers(startSlot, 1, &l_DX11CBuffer.m_CBufferPtr);
		break;
	case ShaderType::FRAGMENT:
		DX11RenderingSystemComponent::get().m_deviceContext->PSSetConstantBuffers(startSlot, 1, &l_DX11CBuffer.m_CBufferPtr);
		break;
	default:
		break;
	}
}

void DX11RenderingSystemNS::cleanRTV(vec4 color, ID3D11RenderTargetView* RTV)
{
	float l_color[4];

	// Setup the color to clear the buffer to.
	l_color[0] = color.x;
	l_color[1] = color.y;
	l_color[2] = color.z;
	l_color[3] = color.w;

	DX11RenderingSystemComponent::get().m_deviceContext->ClearRenderTargetView(RTV, l_color);
}

void DX11RenderingSystemNS::cleanDSV(ID3D11DepthStencilView* DSV)
{
	DX11RenderingSystemComponent::get().m_deviceContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}
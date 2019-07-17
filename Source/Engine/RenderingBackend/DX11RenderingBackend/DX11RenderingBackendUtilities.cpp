#include "DX11RenderingBackendUtilities.h"

#include "../../Component/WinWindowSystemComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE DX11RenderingBackendNS
{
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);
	ID3D10Blob* loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath);

	bool initializeVertexShader(DX11ShaderProgramComponent* rhs, const std::wstring& VSShaderPath);
	bool createVertexShader(ID3D10Blob* shaderBuffer, ID3D11VertexShader** vertexShader);
	bool createInputLayout(ID3D10Blob* shaderBuffer, ID3D11InputLayout** inputLayout);
	bool initializePixelShader(DX11ShaderProgramComponent* rhs, const std::wstring& PSShaderPath);
	bool createPixelShader(ID3D10Blob* shaderBuffer, ID3D11PixelShader** pixelShader);
	bool initializeComputeShader(DX11ShaderProgramComponent* rhs, const std::wstring& CSShaderPath);
	bool createComputeShader(ID3D10Blob* shaderBuffer, ID3D11ComputeShader** computeShader);

	bool submitGPUData(DX11MeshDataComponent* rhs);

	D3D11_TEXTURE2D_DESC getDX11TextureDataDesc(TextureDataDesc textureDataDesc);
	DXGI_FORMAT getTextureFormat(TextureDataDesc textureDataDesc);
	unsigned int getTextureMipLevels(TextureDataDesc textureDataDesc);
	unsigned int getTextureBindFlags(TextureDataDesc textureDataDesc);

	bool submitGPUData(DX11TextureDataComponent* rhs);

	bool submitGPUData(DX11MaterialDataComponent * rhs);

	std::unordered_map<InnoEntity*, DX11MeshDataComponent*> m_initializedDX11Meshes;
	std::unordered_map<InnoEntity*, DX11TextureDataComponent*> m_initializedDX11Textures;
	std::unordered_map<InnoEntity*, DX11MaterialDataComponent*> m_initializedDX11Materials;

	void* m_DX11RenderPassComponentPool;
	void* m_DX11ShaderProgramComponentPool;

	const std::wstring m_shaderRelativePath = L"Res//Shaders//";
}

bool DX11RenderingBackendNS::initializeComponentPool()
{
	m_DX11RenderPassComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(DX11RenderPassComponent), 128);
	m_DX11ShaderProgramComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(DX11ShaderProgramComponent), 256);

	return true;
}

ID3D10Blob* DX11RenderingBackendNS::loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath)
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
	case ShaderType::COMPUTE:
		l_shaderTypeName = "cs_5_0";
		break;
	default:
		break;
	}

	auto l_workingDir = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_workingDirW = std::wstring(l_workingDir.begin(), l_workingDir.end());
	result = D3DCompileFromFile((l_workingDirW + m_shaderRelativePath + shaderFilePath).c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", l_shaderTypeName.c_str(), D3D10_SHADER_ENABLE_STRICTNESS, 0,
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
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "Shader creation failed: cannot find shader!");
		}

		return nullptr;
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingBackend: innoShader: " + l_shaderName + " Shader has been compiled.");
	return l_shaderBuffer;
}

DX11ConstantBuffer DX11RenderingBackendNS::createConstantBuffer(size_t elementSize, size_t elementCount, const std::string& name)
{
	DX11ConstantBuffer l_result;
	l_result.elementSize = elementSize;
	l_result.elementCount = elementCount;

	l_result.m_ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	l_result.m_ConstantBufferDesc.ByteWidth = (unsigned int)(l_result.elementSize * l_result.elementCount);
	l_result.m_ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	l_result.m_ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	l_result.m_ConstantBufferDesc.MiscFlags = 0;
	l_result.m_ConstantBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer
	auto l_hResult = DX11RenderingBackendComponent::get().m_device->CreateBuffer(&l_result.m_ConstantBufferDesc, NULL, &l_result.m_ConstantBufferPtr);

	if (FAILED(l_hResult))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create constant buffer pointer!");
	}

	// Set name
	l_hResult = l_result.m_ConstantBufferPtr->SetPrivateData(WKPDID_D3DDebugObjectName, (unsigned int)name.size(), name.c_str());

	if (FAILED(l_hResult))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't set name for constant buffer!");
	}

	return l_result;
}

bool DX11RenderingBackendNS::createStructuredBuffer(void* initialData, DX11StructuredBuffer& arg, const std::string& name)
{
	if (arg.m_StructuredBufferDesc.ByteWidth > 0)
	{
		D3D11_SUBRESOURCE_DATA subResourceData;
		std::vector<unsigned char> l_initialData(arg.m_StructuredBufferDesc.ByteWidth);

		if (initialData)
		{
			subResourceData.pSysMem = initialData;
		}
		else
		{
			subResourceData.pSysMem = l_initialData.data();
		}

		subResourceData.SysMemPitch = 0;
		subResourceData.SysMemSlicePitch = 0;

		// Create the structured buffer pointer
		auto l_hResult = DX11RenderingBackendComponent::get().m_device->CreateBuffer(&arg.m_StructuredBufferDesc, &subResourceData, &arg.m_StructuredBufferPtr);

		if (FAILED(l_hResult))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create structured buffer pointer!");
			return false;
		}

		// Set name
		l_hResult = arg.m_StructuredBufferPtr->SetPrivateData(WKPDID_D3DDebugObjectName, (unsigned int)name.size(), name.c_str());

		if (FAILED(l_hResult))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't set name for structured buffer!");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = arg.elementCount;

		l_hResult = DX11RenderingBackendComponent::get().m_device->CreateShaderResourceView(arg.m_StructuredBufferPtr, &srvDesc, &arg.SRV);

		if (FAILED(l_hResult))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create SRV!");
			return false;
		}

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = arg.elementCount;
		uavDesc.Buffer.Flags = 0;

		l_hResult = DX11RenderingBackendComponent::get().m_device->CreateUnorderedAccessView(arg.m_StructuredBufferPtr, &uavDesc, &arg.UAV);

		if (FAILED(l_hResult))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create UAV!");
			return false;
		}

		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: structured buffer byte width is 0!");
		return false;
	}
}

bool DX11RenderingBackendNS::destroyStructuredBuffer(DX11StructuredBuffer& arg)
{
	arg.m_StructuredBufferPtr->Release();
	arg.SRV->Release();
	arg.UAV->Release();

	return true;
}

bool DX11RenderingBackendNS::initializeVertexShader(DX11ShaderProgramComponent* rhs, const std::wstring& VSShaderPath)
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

bool DX11RenderingBackendNS::createVertexShader(ID3D10Blob* shaderBuffer, ID3D11VertexShader** vertexShader)
{
	auto result = DX11RenderingBackendComponent::get().m_device->CreateVertexShader(
		shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
		NULL,
		vertexShader);

	if (FAILED(result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create vertex shader!");
		return false;
	}

	return true;
}

bool DX11RenderingBackendNS::createInputLayout(ID3D10Blob* shaderBuffer, ID3D11InputLayout** inputLayout)
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
	auto result = DX11RenderingBackendComponent::get().m_device->CreateInputLayout(
		l_polygonLayout, l_numElements, shaderBuffer->GetBufferPointer(),
		shaderBuffer->GetBufferSize(), inputLayout);

	if (FAILED(result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create vertex shader layout!");
		return false;
	}

	return true;
}

bool DX11RenderingBackendNS::initializePixelShader(DX11ShaderProgramComponent* rhs, const std::wstring& PSShaderPath)
{
	// Compile the shader code.
	auto l_shaderBuffer = loadShaderBuffer(ShaderType::FRAGMENT, PSShaderPath);

	if (!createPixelShader(l_shaderBuffer, &rhs->m_pixelShader))
	{
		return false;
	}

	// Create the texture sampler state.
	auto result = DX11RenderingBackendComponent::get().m_device->CreateSamplerState(
		&rhs->m_samplerDesc,
		&rhs->m_samplerState);

	if (FAILED(result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create texture sampler state!");
		return false;
	}

	l_shaderBuffer->Release();

	return true;
}

bool DX11RenderingBackendNS::createPixelShader(ID3D10Blob* shaderBuffer, ID3D11PixelShader** pixelShader)
{
	auto result = DX11RenderingBackendComponent::get().m_device->CreatePixelShader(
		shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
		NULL,
		pixelShader);

	if (FAILED(result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create pixel shader!");
		return false;
	}

	return true;
}

bool DX11RenderingBackendNS::initializeComputeShader(DX11ShaderProgramComponent* rhs, const std::wstring& CSShaderPath)
{
	// Compile the shader code.
	auto l_shaderBuffer = loadShaderBuffer(ShaderType::COMPUTE, CSShaderPath);

	if (!createComputeShader(l_shaderBuffer, &rhs->m_computeShader))
	{
		return false;
	}

	l_shaderBuffer->Release();

	return true;
}

bool DX11RenderingBackendNS::createComputeShader(ID3D10Blob* shaderBuffer, ID3D11ComputeShader** computeShader)
{
	auto result = DX11RenderingBackendComponent::get().m_device->CreateComputeShader(
		shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(),
		NULL,
		computeShader);

	if (FAILED(result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create compute shader!");
		return false;
	}

	return true;
}

DX11ShaderProgramComponent* DX11RenderingBackendNS::addDX11ShaderProgramComponent(EntityID rhs)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_DX11ShaderProgramComponentPool, sizeof(DX11ShaderProgramComponent));
	auto l_DXSPC = new(l_rawPtr)DX11ShaderProgramComponent();
	l_DXSPC->m_parentEntity = rhs;
	return l_DXSPC;
}

bool DX11RenderingBackendNS::initializeDX11ShaderProgramComponent(DX11ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths)
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
	if (shaderFilePaths.m_CSPath != "")
	{
		l_result = l_result && initializeComputeShader(rhs, std::wstring(shaderFilePaths.m_CSPath.begin(), shaderFilePaths.m_CSPath.end()));
	}
	return l_result;
}

void DX11RenderingBackendNS::OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename)
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
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: innoShader: " + shaderFilename + " compile error: " + errorSStream.str() + "\n -- --------------------------------------------------- -- ");
}

DX11RenderPassComponent* DX11RenderingBackendNS::addDX11RenderPassComponent(const EntityID& parentEntity, const char* name)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_DX11RenderPassComponentPool, sizeof(DX11RenderPassComponent));
	auto l_DXRPC = new(l_rawPtr)DX11RenderPassComponent();
	l_DXRPC->m_parentEntity = parentEntity;
	l_DXRPC->m_name = name;
	return l_DXRPC;
}

bool DX11RenderingBackendNS::initializeDX11RenderPassComponent(DX11RenderPassComponent* DXRPC)
{
	bool l_result = true;

	l_result &= reserveRenderTargets(DXRPC);

	l_result &= createRenderTargets(DXRPC);

	l_result &= createRTV(DXRPC);

	if (DXRPC->m_renderPassDesc.useDepthAttachment)
	{
		l_result &= createDSV(DXRPC);
	}

	l_result &= setupPipeline(DXRPC);

	DXRPC->m_objectStatus = ObjectStatus::Activated;

	return l_result;
}

bool DX11RenderingBackendNS::reserveRenderTargets(DX11RenderPassComponent* DXRPC)
{
	DXRPC->m_RTVs.reserve(DXRPC->m_renderPassDesc.RTNumber);
	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		DXRPC->m_RTVs.emplace_back();
	}

	DXRPC->m_DXTDCs.reserve(DXRPC->m_renderPassDesc.RTNumber);
	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		DXRPC->m_DXTDCs.emplace_back();
	}

	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		DXRPC->m_DXTDCs[i] = addDX11TextureDataComponent();
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingBackend: " + std::string(DXRPC->m_name.c_str()) + " render targets have been allocated.");

	return true;
}

bool DX11RenderingBackendNS::createRenderTargets(DX11RenderPassComponent* DXRPC)
{
	for (unsigned int i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		auto l_TDC = DXRPC->m_DXTDCs[i];

		l_TDC->m_textureDataDesc = DXRPC->m_renderPassDesc.RTDesc;

		l_TDC->m_textureData = nullptr;

		initializeDX11TextureDataComponent(l_TDC);
	}

	if (DXRPC->m_renderPassDesc.useDepthAttachment)
	{
		DXRPC->m_depthStencilDXTDC = addDX11TextureDataComponent();
		DXRPC->m_depthStencilDXTDC->m_textureDataDesc = DXRPC->m_renderPassDesc.RTDesc;

		if (DXRPC->m_renderPassDesc.useStencilAttachment)
		{
			DXRPC->m_depthStencilDXTDC->m_textureDataDesc.usageType = TextureUsageType::DEPTH_STENCIL_ATTACHMENT;
		}
		else
		{
			DXRPC->m_depthStencilDXTDC->m_textureDataDesc.usageType = TextureUsageType::DEPTH_ATTACHMENT;
		}

		DXRPC->m_depthStencilDXTDC->m_textureData = { nullptr };

		initializeDX11TextureDataComponent(DXRPC->m_depthStencilDXTDC);
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingBackend: " + std::string(DXRPC->m_name.c_str()) + " render targets have been created.");

	return true;
}

bool DX11RenderingBackendNS::createRTV(DX11RenderPassComponent* DXRPC)
{
	DXRPC->m_RTVDesc.Format = DXRPC->m_DXTDCs[0]->m_DX11TextureDataDesc.Format;
	DXRPC->m_RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	DXRPC->m_RTVDesc.Texture2D.MipSlice = 0;

	for (unsigned int i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		auto l_result = DX11RenderingBackendComponent::get().m_device->CreateRenderTargetView(DXRPC->m_DXTDCs[i]->m_texture, &DXRPC->m_RTVDesc, &DXRPC->m_RTVs[i]);
		if (FAILED(l_result))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create RTV for " + std::string(DXRPC->m_name.c_str()) + "!");
			return false;
		}
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingBackend: " + std::string(DXRPC->m_name.c_str()) + " RTV has been created.");

	return true;
}

bool DX11RenderingBackendNS::createDSV(DX11RenderPassComponent* DXRPC)
{
	if (DXRPC->m_renderPassDesc.useStencilAttachment)
	{
		DXRPC->m_DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		DXRPC->m_DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
	}

	DXRPC->m_DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DXRPC->m_DSVDesc.Texture2D.MipSlice = 0;

	auto l_result = DX11RenderingBackendComponent::get().m_device->CreateDepthStencilView(DXRPC->m_depthStencilDXTDC->m_texture, &DXRPC->m_DSVDesc, &DXRPC->m_DSV);

	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create the DSV for " + std::string(DXRPC->m_name.c_str()) + "!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingBackend: " + std::string(DXRPC->m_name.c_str()) + " DSV has been created.");

	return true;
}

bool DX11RenderingBackendNS::setupPipeline(DX11RenderPassComponent* DXRPC)
{
	if (DXRPC->m_renderPassDesc.useDepthAttachment)
	{
		DXRPC->m_depthStencilDesc.DepthEnable = true;
		if (DXRPC->m_renderPassDesc.useStencilAttachment)
		{
			DXRPC->m_depthStencilDesc.StencilEnable = true;
		}

		auto l_result = DX11RenderingBackendComponent::get().m_device->CreateDepthStencilState(&DXRPC->m_depthStencilDesc, &DXRPC->m_depthStencilState);
		if (FAILED(l_result))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create the depth stencil state for for " + std::string(DXRPC->m_name.c_str()) + "!");
			return false;
		}
	}

	// Setup the viewport.
	DXRPC->m_viewport.Width = (float)DXRPC->m_renderPassDesc.RTDesc.width;
	DXRPC->m_viewport.Height = (float)DXRPC->m_renderPassDesc.RTDesc.height;
	DXRPC->m_viewport.MinDepth = 0.0f;
	DXRPC->m_viewport.MaxDepth = 1.0f;
	DXRPC->m_viewport.TopLeftX = 0.0f;
	DXRPC->m_viewport.TopLeftY = 0.0f;

	// Create the rasterizer state.
	auto l_result = DX11RenderingBackendComponent::get().m_device->CreateRasterizerState(&DXRPC->m_rasterizerDesc, &DXRPC->m_rasterizerState);
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create the rasterizer state for " + std::string(DXRPC->m_name.c_str()) + "!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX11RenderingBackend: " + std::string(DXRPC->m_name.c_str()) + " pipeline state has been setup.");

	return true;
}

bool DX11RenderingBackendNS::initializeDX11MeshDataComponent(DX11MeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		submitGPUData(rhs);

		return true;
	}
}

bool DX11RenderingBackendNS::submitGPUData(DX11MeshDataComponent * rhs)
{
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
	D3D11_SUBRESOURCE_DATA vertexData;
	ZeroMemory(&vertexData, sizeof(vertexData));
	vertexData.pSysMem = &rhs->m_vertices[0];
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	HRESULT result;
	result = DX11RenderingBackendComponent::get().m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &rhs->m_vertexBuffer);
	if (FAILED(result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create vertex Buffer!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingBackend: vertex Buffer: " + InnoUtility::pointerToString(rhs->m_vertexBuffer) + " is initialized.");

	// Set up the description of the static index buffer.
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = (UINT)(rhs->m_indices.size() * sizeof(unsigned int));
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
	result = DX11RenderingBackendComponent::get().m_device->CreateBuffer(&indexBufferDesc, &indexData, &rhs->m_indexBuffer);
	if (FAILED(result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create index Buffer!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingBackend: index Buffer: " + InnoUtility::pointerToString(rhs->m_indexBuffer) + " is initialized.");

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedDX11Meshes.emplace(rhs->m_parentEntity, rhs);

	return true;
}

bool DX11RenderingBackendNS::initializeDX11TextureDataComponent(DX11TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		if (rhs->m_textureDataDesc.usageType == TextureUsageType::INVISIBLE)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "DX11RenderingBackend: try to generate DX11TextureDataComponent for TextureUsageType::INVISIBLE type!");
			return false;
		}
		else if (rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
		{
			submitGPUData(rhs);
			return true;
		}
		else
		{
			if (rhs->m_textureData)
			{
				submitGPUData(rhs);

				return true;
			}
			else
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "DX11RenderingBackend: try to generate DX11TextureDataComponent without raw data!");
				return false;
			}
		}
	}
}

DXGI_FORMAT DX11RenderingBackendNS::getTextureFormat(TextureDataDesc textureDataDesc)
{
	DXGI_FORMAT l_internalFormat = DXGI_FORMAT_UNKNOWN;

	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_internalFormat = DXGI_FORMAT_R32_TYPELESS;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_internalFormat = DXGI_FORMAT_R24G8_TYPELESS;
	}
	else
	{
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			default: break;
			}
		}
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
}

unsigned int DX11RenderingBackendNS::getTextureMipLevels(TextureDataDesc textureDataDesc)
{
	unsigned int textureMipLevels = 1;
	if (textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		textureMipLevels = 0;
	}

	return textureMipLevels;
}

unsigned int DX11RenderingBackendNS::getTextureBindFlags(TextureDataDesc textureDataDesc)
{
	unsigned int textureBindFlags = 0;
	if (textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	}
	else if (textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	}
	else
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE;
	}

	return textureBindFlags;
}

D3D11_TEXTURE2D_DESC DX11RenderingBackendNS::getDX11TextureDataDesc(TextureDataDesc textureDataDesc)
{
	D3D11_TEXTURE2D_DESC DX11TextureDataDesc = {};

	DX11TextureDataDesc.Height = textureDataDesc.height;
	DX11TextureDataDesc.Width = textureDataDesc.width;
	DX11TextureDataDesc.MipLevels = getTextureMipLevels(textureDataDesc);
	DX11TextureDataDesc.ArraySize = 1;
	DX11TextureDataDesc.Format = getTextureFormat(textureDataDesc);
	DX11TextureDataDesc.SampleDesc.Count = 1;
	DX11TextureDataDesc.Usage = D3D11_USAGE_DEFAULT;
	DX11TextureDataDesc.BindFlags = getTextureBindFlags(textureDataDesc);
	DX11TextureDataDesc.CPUAccessFlags = 0;

	if (textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		DX11TextureDataDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	else
	{
		DX11TextureDataDesc.MiscFlags = 0;
	}

	return DX11TextureDataDesc;
}

bool DX11RenderingBackendNS::submitGPUData(DX11TextureDataComponent * rhs)
{
	rhs->m_DX11TextureDataDesc = getDX11TextureDataDesc(rhs->m_textureDataDesc);

	// Create the empty texture.
	HRESULT hResult;

	hResult = DX11RenderingBackendComponent::get().m_device->CreateTexture2D(&rhs->m_DX11TextureDataDesc, NULL, (ID3D11Texture2D**)&rhs->m_texture);
	if (FAILED(hResult))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create texture!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingBackend: Texture: " + InnoUtility::pointerToString(rhs->m_texture) + " is initialized.");

	// Submit raw data to GPU memory
	if (rhs->m_textureDataDesc.usageType != TextureUsageType::COLOR_ATTACHMENT
		&& rhs->m_textureDataDesc.usageType != TextureUsageType::DEPTH_ATTACHMENT
		&& rhs->m_textureDataDesc.usageType != TextureUsageType::DEPTH_STENCIL_ATTACHMENT
		&& rhs->m_textureDataDesc.usageType != TextureUsageType::RAW_IMAGE)
	{
		unsigned int rowPitch;
		rowPitch = (rhs->m_textureDataDesc.width * ((unsigned int)rhs->m_textureDataDesc.pixelDataFormat + 1)) * sizeof(unsigned char);
		DX11RenderingBackendComponent::get().m_deviceContext->UpdateSubresource(rhs->m_texture, 0, NULL, rhs->m_textureData, rowPitch, 0);
	}

	// Get SRV desc
	if (rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		rhs->m_SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	}
	else if (rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		rhs->m_SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	else
	{
		rhs->m_SRVDesc.Format = rhs->m_DX11TextureDataDesc.Format;
	}

	rhs->m_SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rhs->m_SRVDesc.Texture2D.MostDetailedMip = 0;

	if (rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
		|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
		|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
		|| rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		rhs->m_SRVDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		rhs->m_SRVDesc.Texture2D.MipLevels = -1;
	}

	// Create SRV
	hResult = DX11RenderingBackendComponent::get().m_device->CreateShaderResourceView(rhs->m_texture, &rhs->m_SRVDesc, &rhs->m_SRV);
	if (FAILED(hResult))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create SRV for texture!");
		return false;
	}

	// Generate mipmaps for this texture.
	if (rhs->m_textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		DX11RenderingBackendComponent::get().m_deviceContext->GenerateMips(rhs->m_SRV);
	}

	// Create UAV
	if (rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		rhs->m_UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		rhs->m_UAVDesc.Texture2D.MipSlice = 0;

		hResult = DX11RenderingBackendComponent::get().m_device->CreateUnorderedAccessView(rhs->m_texture, &rhs->m_UAVDesc, &rhs->m_UAV);
		if (FAILED(hResult))
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't create UAV for texture!");
			return false;
		}
	}

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedDX11Textures.emplace(rhs->m_parentEntity, rhs);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX11RenderingBackend: SRV: " + InnoUtility::pointerToString(rhs->m_SRV) + " is initialized.");

	return true;
}

bool DX11RenderingBackendNS::initializeDX11MaterialDataComponent(DX11MaterialDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		submitGPUData(rhs);

		return true;
	}
}

bool DX11RenderingBackendNS::submitGPUData(DX11MaterialDataComponent * rhs)
{
	if (rhs->m_normalTexture)
	{
		initializeDX11TextureDataComponent(reinterpret_cast<DX11TextureDataComponent*>(rhs->m_normalTexture));
	}
	if (rhs->m_albedoTexture)
	{
		initializeDX11TextureDataComponent(reinterpret_cast<DX11TextureDataComponent*>(rhs->m_albedoTexture));
	}
	if (rhs->m_metallicTexture)
	{
		initializeDX11TextureDataComponent(reinterpret_cast<DX11TextureDataComponent*>(rhs->m_metallicTexture));
	}
	if (rhs->m_roughnessTexture)
	{
		initializeDX11TextureDataComponent(reinterpret_cast<DX11TextureDataComponent*>(rhs->m_roughnessTexture));
	}
	if (rhs->m_aoTexture)
	{
		initializeDX11TextureDataComponent(reinterpret_cast<DX11TextureDataComponent*>(rhs->m_aoTexture));
	}

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedDX11Materials.emplace(rhs->m_parentEntity, rhs);

	return true;
}

void DX11RenderingBackendNS::drawMesh(DX11MeshDataComponent * DXMDC)
{
	if (DXMDC->m_vertexBuffer)
	{
		// Set vertex buffer stride and offset.
		unsigned int stride = sizeof(Vertex);
		unsigned int offset = 0;

		// Set the vertex buffer to active in the input assembler so it can be rendered.
		DX11RenderingBackendComponent::get().m_deviceContext->IASetVertexBuffers(0, 1, &DXMDC->m_vertexBuffer, &stride, &offset);

		// Set the index buffer to active in the input assembler so it can be rendered.
		DX11RenderingBackendComponent::get().m_deviceContext->IASetIndexBuffer(DXMDC->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Render the triangle.
		DX11RenderingBackendComponent::get().m_deviceContext->DrawIndexed((UINT)DXMDC->m_indicesSize, 0, 0);
	}
}

void DX11RenderingBackendNS::bindTextureForWrite(ShaderType shaderType, unsigned int startSlot, DX11TextureDataComponent* DXTDC)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to texture!");
		break;
	case ShaderType::GEOMETRY:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to texture!");
		break;
	case ShaderType::FRAGMENT:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to texture!");
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetUnorderedAccessViews(startSlot, 1, &DXTDC->m_UAV, nullptr);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::bindTextureForRead(ShaderType shaderType, unsigned int startSlot, DX11TextureDataComponent* DXTDC)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DX11RenderingBackendComponent::get().m_deviceContext->VSSetShaderResources(startSlot, 1, &DXTDC->m_SRV);
		break;
	case ShaderType::GEOMETRY:
		DX11RenderingBackendComponent::get().m_deviceContext->GSSetShaderResources(startSlot, 1, &DXTDC->m_SRV);
		break;
	case ShaderType::FRAGMENT:
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetShaderResources(startSlot, 1, &DXTDC->m_SRV);
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetShaderResources(startSlot, 1, &DXTDC->m_SRV);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::unbindTextureForWrite(ShaderType shaderType, unsigned int startSlot)
{
	ID3D11UnorderedAccessView* l_UAV[] = { nullptr };

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to texture!");
		break;
	case ShaderType::GEOMETRY:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to texture!");
		break;
	case ShaderType::FRAGMENT:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to texture!");
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetUnorderedAccessViews(startSlot, 1, l_UAV, nullptr);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::unbindTextureForRead(ShaderType shaderType, unsigned int startSlot)
{
	ID3D11ShaderResourceView* l_SRV[] = { nullptr };

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DX11RenderingBackendComponent::get().m_deviceContext->VSSetShaderResources(startSlot, 1, l_SRV);
		break;
	case ShaderType::GEOMETRY:
		DX11RenderingBackendComponent::get().m_deviceContext->GSSetShaderResources(startSlot, 1, l_SRV);
		break;
	case ShaderType::FRAGMENT:
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetShaderResources(startSlot, 1, l_SRV);
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetShaderResources(startSlot, 1, l_SRV);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::activateRenderPass(DX11RenderPassComponent * DXRPC)
{
	DX11RenderingBackendComponent::get().m_deviceContext->OMSetRenderTargets((unsigned int)DXRPC->m_RTVs.size(), &DXRPC->m_RTVs[0], DXRPC->m_DSV);
	DX11RenderingBackendComponent::get().m_deviceContext->RSSetViewports(1, &DXRPC->m_viewport);
	DX11RenderingBackendComponent::get().m_deviceContext->RSSetState(DXRPC->m_rasterizerState);

	if (DXRPC->m_renderPassDesc.useDepthAttachment)
	{
		DX11RenderingBackendComponent::get().m_deviceContext->OMSetDepthStencilState(DXRPC->m_depthStencilState, 0x01);
		cleanDSV(DXRPC->m_DSV);
	}

	for (auto i : DXRPC->m_RTVs)
	{
		cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), i);
	}
}

bool DX11RenderingBackendNS::activateShader(DX11ShaderProgramComponent * rhs)
{
	if (rhs->m_vertexShader)
	{
		DX11RenderingBackendComponent::get().m_deviceContext->VSSetShader(rhs->m_vertexShader, NULL, 0);
		DX11RenderingBackendComponent::get().m_deviceContext->IASetInputLayout(rhs->m_inputLayout);
	}
	if (rhs->m_pixelShader)
	{
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetShader(rhs->m_pixelShader, NULL, 0);
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetSamplers(0, 1, &rhs->m_samplerState);
	}
	if (rhs->m_computeShader)
	{
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetShader(rhs->m_computeShader, NULL, 0);
	}

	return true;
}

void DX11RenderingBackendNS::updateConstantBufferImpl(const DX11ConstantBuffer& ConstantBuffer, size_t size, const void* ConstantBufferValue)
{
	HRESULT l_result;
	D3D11_MAPPED_SUBRESOURCE l_mappedResource;

	// Lock the constant buffer so it can be written to.
	l_result = DX11RenderingBackendComponent::get().m_deviceContext->Map(ConstantBuffer.m_ConstantBufferPtr, 0, D3D11_MAP_WRITE_DISCARD, 0, &l_mappedResource);
	if (FAILED(l_result))
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: can't lock the shader buffer!");
		return;
	}

	auto l_dataPtr = l_mappedResource.pData;
	std::memcpy(l_dataPtr, ConstantBufferValue, size);

	// Unlock the constant buffer.
	DX11RenderingBackendComponent::get().m_deviceContext->Unmap(ConstantBuffer.m_ConstantBufferPtr, 0);
}

void DX11RenderingBackendNS::bindConstantBuffer(ShaderType shaderType, unsigned int startSlot, const DX11ConstantBuffer& ConstantBuffer)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DX11RenderingBackendComponent::get().m_deviceContext->VSSetConstantBuffers(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr);
		break;
	case ShaderType::GEOMETRY:
		DX11RenderingBackendComponent::get().m_deviceContext->GSSetConstantBuffers(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr);
		break;
	case ShaderType::FRAGMENT:
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetConstantBuffers(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr);
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetConstantBuffers(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::bindConstantBuffer(ShaderType shaderType, unsigned int startSlot, const DX11ConstantBuffer& ConstantBuffer, unsigned int offset)
{
	unsigned int l_constantCount = (unsigned int)ConstantBuffer.elementSize / 16;
	unsigned int l_firstConstant = offset * l_constantCount;

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DX11RenderingBackendComponent::get().m_deviceContext->VSSetConstantBuffers1(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr, &l_firstConstant, &l_constantCount);
		break;
	case ShaderType::GEOMETRY:
		DX11RenderingBackendComponent::get().m_deviceContext->GSSetConstantBuffers1(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr, &l_firstConstant, &l_constantCount);
		break;
	case ShaderType::FRAGMENT:
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetConstantBuffers1(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr, &l_firstConstant, &l_constantCount);
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetConstantBuffers1(startSlot, 1, &ConstantBuffer.m_ConstantBufferPtr, &l_firstConstant, &l_constantCount);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::bindStructuredBufferForWrite(ShaderType shaderType, unsigned int startSlot, const DX11StructuredBuffer& StructuredBuffer)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to structured buffer!");
		break;
	case ShaderType::GEOMETRY:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to structured buffer!");
		break;
	case ShaderType::FRAGMENT:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to structured buffer!");
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetUnorderedAccessViews(startSlot, 1, &StructuredBuffer.UAV, nullptr);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::bindStructuredBufferForRead(ShaderType shaderType, unsigned int startSlot, const DX11StructuredBuffer& StructuredBuffer)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DX11RenderingBackendComponent::get().m_deviceContext->VSSetShaderResources(startSlot, 1, &StructuredBuffer.SRV);
		break;
	case ShaderType::GEOMETRY:
		DX11RenderingBackendComponent::get().m_deviceContext->GSSetShaderResources(startSlot, 1, &StructuredBuffer.SRV);
		break;
	case ShaderType::FRAGMENT:
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetShaderResources(startSlot, 1, &StructuredBuffer.SRV);
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetShaderResources(startSlot, 1, &StructuredBuffer.SRV);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::unbindStructuredBufferForWrite(ShaderType shaderType, unsigned int startSlot)
{
	ID3D11UnorderedAccessView* l_UAV[] = { nullptr };

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to structured buffer!");
		break;
	case ShaderType::GEOMETRY:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to structured buffer!");
		break;
	case ShaderType::FRAGMENT:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingBackend: Only compute shader support write to structured buffer!");
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetUnorderedAccessViews(startSlot, 1, l_UAV, nullptr);
		break;
	default:
		break;
	}
}

void DX11RenderingBackendNS::unbindStructuredBufferForRead(ShaderType shaderType, unsigned int startSlot)
{
	ID3D11ShaderResourceView* l_SRV[] = { nullptr };

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		DX11RenderingBackendComponent::get().m_deviceContext->VSSetShaderResources(startSlot, 1, l_SRV);
		break;
	case ShaderType::GEOMETRY:
		DX11RenderingBackendComponent::get().m_deviceContext->GSSetShaderResources(startSlot, 1, l_SRV);
		break;
	case ShaderType::FRAGMENT:
		DX11RenderingBackendComponent::get().m_deviceContext->PSSetShaderResources(startSlot, 1, l_SRV);
		break;
	case ShaderType::COMPUTE:
		DX11RenderingBackendComponent::get().m_deviceContext->CSSetShaderResources(startSlot, 1, l_SRV);
		break;
	default:
		break;
	}
}
void DX11RenderingBackendNS::cleanRTV(vec4 color, ID3D11RenderTargetView* RTV)
{
	float l_color[4];

	// Setup the color to clear the buffer to.
	l_color[0] = color.x;
	l_color[1] = color.y;
	l_color[2] = color.z;
	l_color[3] = color.w;

	DX11RenderingBackendComponent::get().m_deviceContext->ClearRenderTargetView(RTV, l_color);
}

void DX11RenderingBackendNS::cleanDSV(ID3D11DepthStencilView* DSV)
{
	DX11RenderingBackendComponent::get().m_deviceContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0x00);
}
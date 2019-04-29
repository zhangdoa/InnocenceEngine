#include "DX12RenderingSystemUtilities.h"

#include "../../component/WinWindowSystemComponent.h"
#include "../../component/DX12RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DX12RenderingSystemNS
{
	ID3D12GraphicsCommandList* beginSingleTimeCommands();
	void endSingleTimeCommands(ID3D12GraphicsCommandList* commandList);

	void OutputShaderErrorMessage(ID3DBlob * errorMessage, HWND hwnd, const std::string & shaderFilename);
	ID3DBlob* loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath);
	bool initializeVertexShader(DX12ShaderProgramComponent* rhs, const std::wstring& VSShaderPath);
	bool initializePixelShader(DX12ShaderProgramComponent* rhs, const std::wstring& PSShaderPath);

	bool summitGPUData(DX12MeshDataComponent* rhs);

	D3D12_RESOURCE_DESC getDX12TextureDataDesc(TextureDataDesc textureDataDesc);
	DXGI_FORMAT getTextureFormat(TextureDataDesc textureDataDesc);
	unsigned int getTextureMipLevels(TextureDataDesc textureDataDesc);
	D3D12_RESOURCE_FLAGS getTextureBindFlags(TextureDataDesc textureDataDesc);

	bool summitGPUData(DX12TextureDataComponent* rhs);

	std::unordered_map<EntityID, DX12MeshDataComponent*> m_initializedDXMDC;
	std::unordered_map<EntityID, DX12TextureDataComponent*> m_initializedDXTDC;

	std::unordered_map<EntityID, DX12MeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, DX12TextureDataComponent*> m_textureMap;

	void* m_DX12RenderPassComponentPool;
	void* m_DX12ShaderProgramComponentPool;

	const std::wstring m_shaderRelativePath = L"//res//shaders//";
}

bool DX12RenderingSystemNS::initializeComponentPool()
{
	m_DX12RenderPassComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX12RenderPassComponent), 128);
	m_DX12ShaderProgramComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DX12ShaderProgramComponent), 256);

	return true;
}

ID3DBlob* DX12RenderingSystemNS::loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath)
{
	auto l_shaderName = std::string(shaderFilePath.begin(), shaderFilePath.end());
	std::reverse(l_shaderName.begin(), l_shaderName.end());
	l_shaderName = l_shaderName.substr(l_shaderName.find(".") + 1, l_shaderName.find("//") - l_shaderName.find(".") - 1);
	std::reverse(l_shaderName.begin(), l_shaderName.end());

	HRESULT result;
	ID3DBlob* l_errorMessage;
	ID3DBlob* l_shaderBuffer;

	std::string l_shaderTypeName;

	switch (shaderType)
	{
	case ShaderType::VERTEX:
		l_shaderTypeName = "vs_5_1";
		break;
	case ShaderType::GEOMETRY:
		l_shaderTypeName = "gs_5_1";
		break;
	case ShaderType::FRAGMENT:
		l_shaderTypeName = "ps_5_1";
		break;
	default:
		break;
	}

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	auto l_workingDir = g_pCoreSystem->getFileSystem()->getWorkingDirectory();
	auto l_workingDirW = std::wstring(l_workingDir.begin(), l_workingDir.end());

	result = D3DCompileFromFile((l_workingDirW + m_shaderRelativePath + shaderFilePath).c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", l_shaderTypeName.c_str(), compileFlags, 0,
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
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX12RenderingSystem: innoShader: " + l_shaderName + " Shader has been compiled.");
	return l_shaderBuffer;
}

bool DX12RenderingSystemNS::createConstantBuffer(DX12ConstantBuffer& arg, const std::wstring& name)
{
	arg.m_CBVHeapDesc.NumDescriptors = 1;
	arg.m_CBVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	arg.m_CBVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto l_result = DX12RenderingSystemComponent::get().m_device->CreateDescriptorHeap(&arg.m_CBVHeapDesc, IID_PPV_ARGS(&arg.m_CBVHeap));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create DescriptorHeap for CBV!");
		return false;
	}

	arg.m_CBVHandle = arg.m_CBVHeap->GetCPUDescriptorHandleForHeapStart();

	l_result = DX12RenderingSystemComponent::get().m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(arg.m_CBVDesc.SizeInBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&arg.m_ConstantBufferPtr));

	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't create ConstantBuffer!");
		return false;
	}

	CD3DX12_RANGE constantBufferReadRange(0, 0);
	arg.m_ConstantBufferPtr->Map(0, &constantBufferReadRange, &arg.m_mappedPtr);
	arg.m_ConstantBufferPtr->SetName(name.c_str());

	arg.m_CBVDesc.BufferLocation = arg.m_ConstantBufferPtr->GetGPUVirtualAddress();

	DX12RenderingSystemComponent::get().m_device->CreateConstantBufferView(&arg.m_CBVDesc, arg.m_CBVHandle);

	return true;
}

void DX12RenderingSystemNS::updateConstantBuffer(const DX12ConstantBuffer& ConstantBuffer, void* ConstantBufferValue)
{
	std::memcpy(ConstantBuffer.m_mappedPtr, &ConstantBufferValue, ConstantBuffer.m_CBVDesc.SizeInBytes);
}

bool DX12RenderingSystemNS::initializeVertexShader(DX12ShaderProgramComponent* rhs, const std::wstring& VSShaderPath)
{
	// Compile the shader code.
	auto l_shaderBuffer = loadShaderBuffer(ShaderType::VERTEX, VSShaderPath);

	rhs->m_vertexShader = l_shaderBuffer;

	return true;
}

bool DX12RenderingSystemNS::initializePixelShader(DX12ShaderProgramComponent* rhs, const std::wstring& PSShaderPath)
{
	// Compile the shader code.
	auto l_shaderBuffer = loadShaderBuffer(ShaderType::FRAGMENT, PSShaderPath);

	rhs->m_pixelShader = l_shaderBuffer;

	return true;
}

DX12ShaderProgramComponent* DX12RenderingSystemNS::addDX12ShaderProgramComponent(EntityID rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_DX12ShaderProgramComponentPool, sizeof(DX12ShaderProgramComponent));
	auto l_DXSPC = new(l_rawPtr)DX12ShaderProgramComponent();
	l_DXSPC->m_parentEntity = rhs;
	return l_DXSPC;
}

bool DX12RenderingSystemNS::initializeDX12ShaderProgramComponent(DX12ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths)
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

	return l_result;
}

ID3D12GraphicsCommandList* DX12RenderingSystemNS::beginSingleTimeCommands()
{
	ID3D12GraphicsCommandList* l_commandList;

	// Create a basic command list.
	auto hResult = DX12RenderingSystemComponent::get().m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, DX12RenderingSystemComponent::get().m_commandAllocator, NULL, IID_PPV_ARGS(&l_commandList));
	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create command list!");
		return nullptr;
	}

	return l_commandList;
}

void DX12RenderingSystemNS::endSingleTimeCommands(ID3D12GraphicsCommandList* commandList)
{
	auto hResult = commandList->Close();
	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't close the command list!");
	}

	ID3D12Fence1* l_uploadFinishFence;
	hResult = DX12RenderingSystemComponent::get().m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&l_uploadFinishFence));
	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create fence!");
	}

	auto l_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (l_fenceEvent == nullptr)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create fence event!");
	}

	ID3D12CommandList* ppCommandLists[] = { commandList };
	DX12RenderingSystemComponent::get().m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	DX12RenderingSystemComponent::get().m_commandQueue->Signal(l_uploadFinishFence, 1);
	l_uploadFinishFence->SetEventOnCompletion(1, l_fenceEvent);
	WaitForSingleObject(l_fenceEvent, INFINITE);
	CloseHandle(l_fenceEvent);

	commandList->Release();
}

void DX12RenderingSystemNS::OutputShaderErrorMessage(ID3DBlob * errorMessage, HWND hwnd, const std::string & shaderFilename)
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
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: innoShader: " + shaderFilename + " compile error: " + errorSStream.str() + "\n -- --------------------------------------------------- -- ");
}

bool DX12RenderingSystemNS::activateDX12ShaderProgramComponent(DX12ShaderProgramComponent * rhs)
{
	return true;
}

DX12RenderPassComponent* DX12RenderingSystemNS::addDX12RenderPassComponent(EntityID rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_DX12RenderPassComponentPool, sizeof(DX12RenderPassComponent));
	auto l_DXRPC = new(l_rawPtr)DX12RenderPassComponent();
	l_DXRPC->m_parentEntity = rhs;
	return l_DXRPC;
}

bool DX12RenderingSystemNS::initializeDX12RenderPassComponent(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC)
{
	bool result = true;

	result &= reserveRenderTargets(DXRPC);

	result &= createRenderTargets(DXRPC);

	result &= createRTVDescriptorHeap(DXRPC);
	result &= createRTV(DXRPC);
	result &= createRootSignature(DXRPC);
	result &= createPSO(DXRPC, DXSPC);
	result &= createCommandLists(DXRPC);
	result &= createSyncPrimitives(DXRPC);

	DXRPC->m_objectStatus = ObjectStatus::ALIVE;

	return DXRPC;
}

bool DX12RenderingSystemNS::reserveRenderTargets(DX12RenderPassComponent* DXRPC)
{
	// reserve vectors and emplace empty objects
	DXRPC->m_DXTDCs.reserve(DXRPC->m_renderPassDesc.RTNumber);
	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		DXRPC->m_DXTDCs.emplace_back();
	}

	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		DXRPC->m_DXTDCs[i] = addDX12TextureDataComponent();
	}

	return true;
}

bool DX12RenderingSystemNS::createRenderTargets(DX12RenderPassComponent* DXRPC)
{
	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		auto l_TDC = DXRPC->m_DXTDCs[i];

		l_TDC->m_textureDataDesc = DXRPC->m_renderPassDesc.RTDesc;

		if (l_TDC->m_textureDataDesc.samplerType == TextureSamplerType::CUBEMAP)
		{
			l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		}
		else
		{
			l_TDC->m_textureData = { nullptr };
		}

		initializeDX12TextureDataComponent(l_TDC);
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Render targets have been created.");

	return true;
}

bool DX12RenderingSystemNS::createRTVDescriptorHeap(DX12RenderPassComponent* DXRPC)
{
	if (DXRPC->m_RTVHeapDesc.NumDescriptors)
	{
		auto l_result = DX12RenderingSystemComponent::get().m_device->CreateDescriptorHeap(&DXRPC->m_RTVHeapDesc, IID_PPV_ARGS(&DXRPC->m_RTVHeap));
		if (FAILED(l_result))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create DescriptorHeap for RTV!");
			return false;
		}
		DXRPC->m_RTVDescHandle = DXRPC->m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: RTV DescriptorHeap has been created.");

	return true;
}

bool DX12RenderingSystemNS::createRTV(DX12RenderPassComponent* DXRPC)
{
	auto l_RTVDescSize = DX12RenderingSystemComponent::get().m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	DXRPC->m_RTVDesc.Format = DXRPC->m_DXTDCs[0]->m_DX12TextureDataDesc.Format;

	D3D12_CPU_DESCRIPTOR_HANDLE l_handle = DXRPC->m_RTVDescHandle;

	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		DX12RenderingSystemComponent::get().m_device->CreateRenderTargetView(DXRPC->m_DXTDCs[i]->m_texture, &DXRPC->m_RTVDesc, l_handle);
		l_handle.ptr += l_RTVDescSize;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: RTV has been created.");

	return true;
}

bool DX12RenderingSystemNS::createRootSignature(DX12RenderPassComponent* DXRPC)
{
	ID3DBlob* l_signature;
	ID3DBlob* l_error;

	auto l_result = D3D12SerializeVersionedRootSignature(&DXRPC->m_rootSignatureDesc, &l_signature, &l_error);

	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't serialize RootSignature!");
		return false;
	}

	l_result = DX12RenderingSystemComponent::get().m_device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&DXRPC->m_rootSignature));

	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create Root Signature!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: RootSignature has been created.");

	return true;
}

bool DX12RenderingSystemNS::createPSO(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC)
{
	D3D12_INPUT_ELEMENT_DESC l_polygonLayout[5];
	unsigned int l_numElements;

	// Create the vertex input layout description.
	l_polygonLayout[0].SemanticName = "POSITION";
	l_polygonLayout[0].SemanticIndex = 0;
	l_polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[0].InputSlot = 0;
	l_polygonLayout[0].AlignedByteOffset = 0;
	l_polygonLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	l_polygonLayout[0].InstanceDataStepRate = 0;

	l_polygonLayout[1].SemanticName = "TEXCOORD";
	l_polygonLayout[1].SemanticIndex = 0;
	l_polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[1].InputSlot = 0;
	l_polygonLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	l_polygonLayout[1].InstanceDataStepRate = 0;

	l_polygonLayout[2].SemanticName = "PADA";
	l_polygonLayout[2].SemanticIndex = 0;
	l_polygonLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	l_polygonLayout[2].InputSlot = 0;
	l_polygonLayout[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	l_polygonLayout[2].InstanceDataStepRate = 0;

	l_polygonLayout[3].SemanticName = "NORMAL";
	l_polygonLayout[3].SemanticIndex = 0;
	l_polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[3].InputSlot = 0;
	l_polygonLayout[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	l_polygonLayout[3].InstanceDataStepRate = 0;

	l_polygonLayout[4].SemanticName = "PADB";
	l_polygonLayout[4].SemanticIndex = 0;
	l_polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	l_polygonLayout[4].InputSlot = 0;
	l_polygonLayout[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	l_polygonLayout[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	l_polygonLayout[4].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	l_numElements = sizeof(l_polygonLayout) / sizeof(l_polygonLayout[0]);

	D3D12_SHADER_BYTECODE l_vsBytecode;
	l_vsBytecode.pShaderBytecode = DXSPC->m_vertexShader->GetBufferPointer();
	l_vsBytecode.BytecodeLength = DXSPC->m_vertexShader->GetBufferSize();

	D3D12_SHADER_BYTECODE l_psBytecode;
	l_psBytecode.pShaderBytecode = DXSPC->m_pixelShader->GetBufferPointer();
	l_psBytecode.BytecodeLength = DXSPC->m_pixelShader->GetBufferSize();

	DXRPC->m_PSODesc.pRootSignature = DXRPC->m_rootSignature;
	DXRPC->m_PSODesc.InputLayout = { l_polygonLayout, l_numElements };
	DXRPC->m_PSODesc.VS = l_vsBytecode;
	DXRPC->m_PSODesc.PS = l_psBytecode;
	DXRPC->m_PSODesc.NumRenderTargets = DXRPC->m_renderPassDesc.RTNumber;
	for (size_t i = 0; i < DXRPC->m_renderPassDesc.RTNumber; i++)
	{
		DXRPC->m_PSODesc.RTVFormats[i] = DXRPC->m_DXTDCs[i]->m_DX12TextureDataDesc.Format;
	}

	auto l_result = DX12RenderingSystemComponent::get().m_device->CreateGraphicsPipelineState(&DXRPC->m_PSODesc, IID_PPV_ARGS(&DXRPC->m_PSO));

	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create PSO!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: PSO has been created.");

	return true;
}

bool DX12RenderingSystemNS::createCommandLists(DX12RenderPassComponent* DXRPC)
{
	if (DXRPC->m_renderPassDesc.useMultipleFramebuffers)
	{
		DXRPC->m_commandLists.resize(DXRPC->m_renderPassDesc.RTNumber);
	}
	else
	{
		DXRPC->m_commandLists.resize(1);
	}

	for (size_t i = 0; i < DXRPC->m_commandLists.size(); i++)
	{
		auto hResult = DX12RenderingSystemComponent::get().m_device->CreateCommandList
		(0, D3D12_COMMAND_LIST_TYPE_DIRECT, DX12RenderingSystemComponent::get().m_commandAllocator, NULL, IID_PPV_ARGS(&DXRPC->m_commandLists[i]));
		if (FAILED(hResult))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create CommandList!");
			return false;
		}
		DXRPC->m_commandLists[i]->Close();
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: CommandList has been created.");

	return true;
}

bool DX12RenderingSystemNS::createSyncPrimitives(DX12RenderPassComponent* DXRPC)
{
	HRESULT l_result;

	// Create a fence for GPU synchronization.
	l_result = DX12RenderingSystemComponent::get().m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&DXRPC->m_fence));
	if (FAILED(l_result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create fence!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Fence has been created.");

	// Create an event object for the fence.
	DXRPC->m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (DXRPC->m_fenceEvent == NULL)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: Can't create fence event!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DX12RenderingSystem: Fence event has been created.");

	// Initialize the starting fence value.
	DXRPC->m_fenceValues.reserve(
		DXRPC->m_commandLists.size()
	);

	for (size_t i = 0; i < DXRPC->m_commandLists.size(); i++)
	{
		DXRPC->m_fenceValues.emplace_back();
	}

	return true;
}

bool DX12RenderingSystemNS::initializeDX12MeshDataComponent(DX12MeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return true;
	}
	else
	{
		summitGPUData(rhs);

		rhs->m_objectStatus = ObjectStatus::ALIVE;

		return true;
	}
}

bool DX12RenderingSystemNS::initializeDX12TextureDataComponent(DX12TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return true;
	}
	else
	{
		if (rhs->m_textureDataDesc.usageType == TextureUsageType::INVISIBLE)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "DX12RenderingSystem: try to generate DX12TextureDataComponent for TextureUsageType::INVISIBLE type!");
			return false;
		}
		else
		{
			if (rhs->m_textureData.size() > 0)
			{
				summitGPUData(rhs);

				rhs->m_objectStatus = ObjectStatus::ALIVE;

				if (rhs->m_textureDataDesc.usageType != TextureUsageType::COLOR_ATTACHMENT)
				{
					// @TODO: release raw data in heap memory
				}

				return true;
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "DX12RenderingSystem: try to generate DX12TextureDataComponent without raw data!");
				return false;
			}
		}
	}
}

bool DX12RenderingSystemNS::summitGPUData(DX12MeshDataComponent * rhs)
{
	auto l_verticesDataSize = unsigned int(sizeof(Vertex) * rhs->m_vertices.size());

	auto hResult = DX12RenderingSystemComponent::get().m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&rhs->m_vertexBuffer));

	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't create vertex buffer!");
		return false;
	}

	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE vertexReadRange(0, 0);        // We do not intend to read from this resource on the CPU.
	hResult = rhs->m_vertexBuffer->Map(0, &vertexReadRange, reinterpret_cast<void**>(&pVertexDataBegin));

	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't map vertex buffer device memory!");
		return false;
	}

	std::memcpy(pVertexDataBegin, &rhs->m_vertices[0], l_verticesDataSize);
	rhs->m_vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	rhs->m_vertexBufferView.BufferLocation = rhs->m_vertexBuffer->GetGPUVirtualAddress();
	rhs->m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	rhs->m_vertexBufferView.SizeInBytes = l_verticesDataSize;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX12RenderingSystem: VBO " + InnoUtility::pointerToString(rhs->m_vertexBuffer) + " is initialized.");

	auto l_indicesDataSize = unsigned int(sizeof(Index) * rhs->m_indices.size());
	DX12RenderingSystemComponent::get().m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&rhs->m_indexBuffer));

	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't create index buffer!");
		return false;
	}

	// Copy the indice data to the index buffer.
	UINT8* pIndexDataBegin;
	CD3DX12_RANGE indexReadRange(0, 0);        // We do not intend to read from this resource on the CPU.
	hResult = rhs->m_indexBuffer->Map(0, &indexReadRange, reinterpret_cast<void**>(&pIndexDataBegin));

	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't map index buffer device memory!");
		return false;
	}

	std::memcpy(pIndexDataBegin, &rhs->m_indices[0], l_indicesDataSize);
	rhs->m_indexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view.
	rhs->m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	rhs->m_indexBufferView.BufferLocation = rhs->m_indexBuffer->GetGPUVirtualAddress();
	rhs->m_indexBufferView.SizeInBytes = l_indicesDataSize;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX12RenderingSystem: IBO " + InnoUtility::pointerToString(rhs->m_indexBuffer) + " is initialized.");

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedDXMDC.emplace(rhs->m_parentEntity, rhs);

	return true;
}

DXGI_FORMAT DX12RenderingSystemNS::getTextureFormat(TextureDataDesc textureDataDesc)
{
	DXGI_FORMAT l_internalFormat = DXGI_FORMAT_UNKNOWN;

	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
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

unsigned int DX12RenderingSystemNS::getTextureMipLevels(TextureDataDesc textureDataDesc)
{
	unsigned int textureMipLevels = 1;
	if (textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		textureMipLevels = 0;
	}

	return textureMipLevels;
}

D3D12_RESOURCE_FLAGS DX12RenderingSystemNS::getTextureBindFlags(TextureDataDesc textureDataDesc)
{
	D3D12_RESOURCE_FLAGS textureBindFlags = {};
	if (textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		textureBindFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		textureBindFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	else if (textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		textureBindFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return textureBindFlags;
}

D3D12_RESOURCE_DESC DX12RenderingSystemNS::getDX12TextureDataDesc(TextureDataDesc textureDataDesc)
{
	D3D12_RESOURCE_DESC DX12TextureDataDesc = {};

	DX12TextureDataDesc.Height = textureDataDesc.height;
	DX12TextureDataDesc.Width = textureDataDesc.width;
	DX12TextureDataDesc.MipLevels = getTextureMipLevels(textureDataDesc);
	DX12TextureDataDesc.DepthOrArraySize = 1;
	DX12TextureDataDesc.Format = getTextureFormat(textureDataDesc);
	DX12TextureDataDesc.SampleDesc.Count = 1;
	DX12TextureDataDesc.SampleDesc.Quality = 0;
	DX12TextureDataDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	DX12TextureDataDesc.Flags = getTextureBindFlags(textureDataDesc);

	return DX12TextureDataDesc;
}

bool DX12RenderingSystemNS::summitGPUData(DX12TextureDataComponent * rhs)
{
	rhs->m_DX12TextureDataDesc = getDX12TextureDataDesc(rhs->m_textureDataDesc);

	unsigned int SRVMipLevels = -1;
	if (rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		SRVMipLevels = 1;
	}

	// Create the empty texture.
	HRESULT hResult;
	hResult = DX12RenderingSystemComponent::get().m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rhs->m_DX12TextureDataDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&rhs->m_texture));

	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't create texture!");
		return false;
	}

	auto l_commandList = beginSingleTimeCommands();

	if (rhs->m_textureDataDesc.usageType != TextureUsageType::COLOR_ATTACHMENT)
	{
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(rhs->m_texture, 0, 1);

		ID3D12Resource* textureUploadHeap;

		// Create the GPU upload buffer.
		hResult = DX12RenderingSystemComponent::get().m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureUploadHeap));

		if (FAILED(hResult))
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't create upload buffer!");
			return false;
		}

		// Copy data to the intermediate upload heap and then schedule a copy
		// from the upload heap to the Texture2D.
		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = rhs->m_textureData[0];
		textureData.RowPitch = rhs->m_textureDataDesc.width * ((unsigned int)rhs->m_textureDataDesc.pixelDataFormat + 1);
		textureData.SlicePitch = textureData.RowPitch * rhs->m_textureDataDesc.height;

		UpdateSubresources(l_commandList, rhs->m_texture, textureUploadHeap, 0, 0, 1, &textureData);

		l_commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(rhs->m_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	// Describe and create a shader resource view (SRV) heap for the texture.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hResult = DX12RenderingSystemComponent::get().m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&rhs->m_SRVHeap));

	if (FAILED(hResult))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX12RenderingSystem: can't create SRV heap!");
		return false;
	}

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = rhs->m_DX12TextureDataDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = SRVMipLevels;
	DX12RenderingSystemComponent::get().m_device->CreateShaderResourceView(rhs->m_texture, &srvDesc, rhs->m_SRVHeap->GetCPUDescriptorHandleForHeapStart());

	endSingleTimeCommands(l_commandList);

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedDXTDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "DX12RenderingSystem: SRV " + InnoUtility::pointerToString(rhs->m_SRV) + " is initialized.");

	return true;
}

bool DX12RenderingSystemNS::recordCommandBegin(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex)
{
	DXRPC->m_commandLists[commandListIndex]->Reset(DX12RenderingSystemComponent::get().m_commandAllocator, DXRPC->m_PSO);

	return true;
}

bool DX12RenderingSystemNS::recordActivateRenderPass(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex)
{
	DXRPC->m_commandLists[commandListIndex]->SetGraphicsRootSignature(DXRPC->m_rootSignature);
	DXRPC->m_commandLists[commandListIndex]->RSSetViewports(1, &DXRPC->m_viewport);
	DXRPC->m_commandLists[commandListIndex]->RSSetScissorRects(1, &DXRPC->m_scissor);

	const float l_clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	if (DXRPC->m_renderPassDesc.useMultipleFramebuffers)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE l_RTVHandle(DXRPC->m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), commandListIndex,
			DX12RenderingSystemComponent::get().m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

		DXRPC->m_commandLists[commandListIndex]->OMSetRenderTargets(1, &l_RTVHandle, FALSE, nullptr);
		DXRPC->m_commandLists[commandListIndex]->ClearRenderTargetView(l_RTVHandle, l_clearColor, 0, nullptr);
	}
	else
	{
		DXRPC->m_commandLists[commandListIndex]->OMSetRenderTargets(1, &DXRPC->m_RTVDescHandle, FALSE, nullptr);
		DXRPC->m_commandLists[commandListIndex]->ClearRenderTargetView(DXRPC->m_RTVDescHandle, l_clearColor, 0, nullptr);
	}

	return true;
}

bool DX12RenderingSystemNS::recordDrawCall(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex, DX12MeshDataComponent * DXMDC)
{
	DXRPC->m_commandLists[commandListIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DXRPC->m_commandLists[commandListIndex]->IASetVertexBuffers(0, 1, &DXMDC->m_vertexBufferView);
	DXRPC->m_commandLists[commandListIndex]->IASetIndexBuffer(&DXMDC->m_indexBufferView);
	DXRPC->m_commandLists[commandListIndex]->DrawIndexedInstanced((unsigned int)DXMDC->m_indicesSize, 1, 0, 0, 0);

	return true;
}

bool DX12RenderingSystemNS::recordCommandEnd(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex)
{
	DXRPC->m_commandLists[commandListIndex]->Close();

	return true;
}
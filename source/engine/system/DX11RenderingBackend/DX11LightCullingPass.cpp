#include "DX11LightCullingPass.h"
#include "DX11RenderingSystemUtilities.h"

#include "DX11OpaquePass.h"

#include "../../component/DX11RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingSystemNS;

INNO_PRIVATE_SCOPE DX11LightCullingPass
{
	bool initializeShaders();

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_tileFrustumDXSPC;
	DX11ShaderProgramComponent* m_lightCullingDXSPC;

	ShaderFilePaths m_tileFrustumShaderFilePaths = { "" , "", "", "DX11//tileFrustumCompute.hlsl" };
	ShaderFilePaths m_lightCullingShaderFilePaths = { "" , "", "", "DX11//lightCullingCompute.hlsl" };

	DX11TextureDataComponent* m_lightGridDXTDC;

	EntityID m_entityID;
}

bool DX11LightCullingPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX11RenderPassComponent(1, DX11RenderingSystemComponent::get().deferredPassRTVDesc, DX11RenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer.m_StructuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer.m_StructuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer.m_StructuredBufferDesc.StructureByteStride = 64;
	DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer.m_StructuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer.m_StructuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer.m_StructuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer.m_StructuredBufferDesc.StructureByteStride = sizeof(uint32_t);
	DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer.m_StructuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer.m_StructuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer.m_StructuredBufferDesc.ByteWidth = sizeof(uint32_t);
	DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer.m_StructuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer.m_StructuredBufferDesc.StructureByteStride = sizeof(uint32_t);
	DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer.m_StructuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer.elementCount = 1;

	auto l_initialIndexCount = 1;
	createStructuredBuffer(&l_initialIndexCount, DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer);

	m_lightGridDXTDC = addDX11TextureDataComponent();
	m_lightGridDXTDC->m_textureDataDesc = DX11RenderingSystemComponent::get().deferredPassTextureDesc;
	m_lightGridDXTDC->m_textureDataDesc.usageType = TextureUsageType::RAW_IMAGE;
	m_lightGridDXTDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::RG;
	m_lightGridDXTDC->m_textureDataDesc.pixelDataType = TexturePixelDataType::UINT32;
	m_lightGridDXTDC->m_textureData = { nullptr };
	initializeDX11TextureDataComponent(m_lightGridDXTDC);

	return true;
}

bool DX11LightCullingPass::initializeShaders()
{
	m_tileFrustumDXSPC = addDX11ShaderProgramComponent(m_entityID);
	m_lightCullingDXSPC = addDX11ShaderProgramComponent(m_entityID);

	initializeDX11ShaderProgramComponent(m_tileFrustumDXSPC, m_tileFrustumShaderFilePaths);
	initializeDX11ShaderProgramComponent(m_lightCullingDXSPC, m_lightCullingShaderFilePaths);

	return true;
}

bool DX11LightCullingPass::update()
{
	// Calculate tile frustums
	auto l_viewportSize = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / 16);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / 16);
	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / 16);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / 16);

	auto numThreads = TVec4<unsigned int>((unsigned int)l_numThreadsX, (unsigned int)l_numThreadsY, 1, 0);
	auto numThreadGroups = TVec4<unsigned int>((unsigned int)l_numThreadGroupsX, (unsigned int)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = numThreads.x * numThreads.y * numThreads.z;

	DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer.m_StructuredBufferDesc.ByteWidth = l_elementCount * 64;
	DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer.elementCount = l_elementCount;

	createStructuredBuffer(nullptr, DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer);

	DX11RenderingSystemComponent::get().m_dispatchParamsConstantBufferData.numThreadGroups = numThreadGroups;
	DX11RenderingSystemComponent::get().m_dispatchParamsConstantBufferData.numThreads = numThreads;
	updateConstantBuffer(DX11RenderingSystemComponent::get().m_dispatchParamsConstantBuffer, &DX11RenderingSystemComponent::get().m_dispatchParamsConstantBufferData);

	activateDX11ShaderProgramComponent(m_tileFrustumDXSPC);
	bindConstantBuffer(ShaderType::COMPUTE, 0, DX11RenderingSystemComponent::get().m_dispatchParamsConstantBuffer);
	bindConstantBuffer(ShaderType::COMPUTE, 1, DX11RenderingSystemComponent::get().m_skyConstantBuffer);
	bindStructuredBufferForWrite(ShaderType::COMPUTE, 0, DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer);

	DX11RenderingSystemComponent::get().m_deviceContext->Dispatch(numThreadGroups.x, numThreadGroups.y, numThreadGroups.z);

	// culling lights
	auto l_maxOverlapLight = 16;

	DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer.m_StructuredBufferDesc.ByteWidth = l_elementCount * sizeof(uint32_t) * l_maxOverlapLight;
	DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer.elementCount = l_elementCount * l_maxOverlapLight;

	createStructuredBuffer(nullptr, DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer);

	activateDX11ShaderProgramComponent(m_lightCullingDXSPC);

	bindConstantBuffer(ShaderType::COMPUTE, 0, DX11RenderingSystemComponent::get().m_dispatchParamsConstantBuffer);
	bindConstantBuffer(ShaderType::COMPUTE, 1, DX11RenderingSystemComponent::get().m_skyConstantBuffer);
	bindConstantBuffer(ShaderType::COMPUTE, 2, DX11RenderingSystemComponent::get().m_pointLightConstantBuffer);

	bindStructuredBufferForRead(ShaderType::COMPUTE, 1, DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer);

	bindTextureForRead(ShaderType::COMPUTE, 0, DX11OpaquePass::getDX11RPC()->m_depthStencilDXTDC);

	bindStructuredBufferForWrite(ShaderType::COMPUTE, 0, DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer);
	bindStructuredBufferForWrite(ShaderType::COMPUTE, 1, DX11RenderingSystemComponent::get().m_lightListIndexCounterStructuredBuffer);
	bindTextureForWrite(ShaderType::COMPUTE, 2, m_lightGridDXTDC);

	DX11RenderingSystemComponent::get().m_deviceContext->Dispatch(numThreadGroups.x, numThreadGroups.y, numThreadGroups.z);

	destroyStructuredBuffer(DX11RenderingSystemComponent::get().m_lightIndexListStructuredBuffer);

	destroyStructuredBuffer(DX11RenderingSystemComponent::get().m_gridFrustumsStructuredBuffer);

	return true;
}

bool DX11LightCullingPass::resize()
{
	return true;
}

bool DX11LightCullingPass::reloadShaders()
{
	return true;
}

DX11RenderPassComponent * DX11LightCullingPass::getDX11RPC()
{
	return m_DXRPC;
}
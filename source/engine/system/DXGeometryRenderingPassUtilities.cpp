#include "DXRenderingSystemUtilities.h"
#include "DXGeometryRenderingPassUtilities.h"

#include "../component/DXGeometryRenderPassComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/DXRenderingSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DXRenderingSystemNS;

INNO_PRIVATE_SCOPE DXGeometryRenderingPassUtilities
{
	void initializeOpaquePass();
	void initializeOpaquePassShaders();

	void updateGeometryPass();
	void updateOpaquePass();

	EntityID m_entityID;
}

void DXGeometryRenderingPassUtilities::initialize()
{
	m_entityID = InnoMath::createEntityID();

	initializeOpaquePass();
}

void DXGeometryRenderingPassUtilities::initializeOpaquePass()
{
	DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC = addDXRenderPassComponent(8, DXRenderingSystemComponent::get().deferredPassRTVDesc, DXRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeOpaquePassShaders();
}

void DXGeometryRenderingPassUtilities::initializeOpaquePassShaders()
{
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC = g_pCoreSystem->getMemorySystem()->spawn<DXShaderProgramComponent>();

	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_vertexShaderCBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_vertexShaderCBufferDesc.ByteWidth = sizeof(GPassMeshCBufferData);
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_vertexShaderCBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_vertexShaderCBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_vertexShaderCBufferDesc.MiscFlags = 0;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_vertexShaderCBufferDesc.StructureByteStride = 0;

	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_pixelShaderCBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_pixelShaderCBufferDesc.ByteWidth = sizeof(GPassTextureCBufferData);
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_pixelShaderCBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_pixelShaderCBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_pixelShaderCBufferDesc.MiscFlags = 0;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_pixelShaderCBufferDesc.StructureByteStride = 0;

	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.MinLOD = 0;
	DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	initializeDXShaderProgramComponent(DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC, DXGeometryRenderPassComponent::get().m_opaquePass_shaderFilePaths);
}

void DXGeometryRenderingPassUtilities::update()
{
	updateOpaquePass();
}

void DXGeometryRenderingPassUtilities::updateOpaquePass()
{
	// Set Rasterizer State
	DXRenderingSystemComponent::get().m_deviceContext->RSSetState(
		DXRenderingSystemComponent::get().m_rasterStateForward);

	activateDXShaderProgramComponent(DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DXRenderingSystemComponent::get().m_deviceContext->OMSetRenderTargets(
		(unsigned int)DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_renderTargetViews.size(),
		&DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_renderTargetViews[0],
		DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_depthStencilView);

	// Set the viewport.
	DXRenderingSystemComponent::get().m_deviceContext->RSSetViewports(
		1,
		&DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_viewport);

	// Clear the render buffers.
	for (auto i : DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_renderTargetViews)
	{
		cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), i);
	}
	cleanDSV(DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_depthStencilView);

	// draw
	while (DXRenderingSystemComponent::get().m_GPassRenderingDataQueue.size() > 0)
	{
		auto l_renderPack = DXRenderingSystemComponent::get().m_GPassRenderingDataQueue.front();

		// Set the type of primitive that should be rendered from this vertex buffer.
		D3D_PRIMITIVE_TOPOLOGY l_primitiveTopology;

		if (l_renderPack.meshPrimitiveTopology == MeshPrimitiveTopology::TRIANGLE)
		{
			l_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
		else
		{
			l_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		}

		DXRenderingSystemComponent::get().m_deviceContext->IASetPrimitiveTopology(l_primitiveTopology);

		updateShaderParameter<GPassMeshCBufferData>(ShaderType::VERTEX, DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_vertexShaderCBuffer, &l_renderPack.meshCBuffer);
		updateShaderParameter<GPassTextureCBufferData>(ShaderType::FRAGMENT, DXGeometryRenderPassComponent::get().m_opaquePass_DXSPC->m_pixelShaderCBuffer, &l_renderPack.textureCBuffer);

		// bind to textures
		// any normal?
		if (l_renderPack.textureCBuffer.useNormalTexture)
		{
			DXRenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(0, 1, &l_renderPack.normalDXTDC->m_SRV);
		}
		// any albedo?
		if (l_renderPack.textureCBuffer.useAlbedoTexture)
		{
			DXRenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(1, 1, &l_renderPack.albedoDXTDC->m_SRV);
		}
		// any metallic?
		if (l_renderPack.textureCBuffer.useMetallicTexture)
		{
			DXRenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(2, 1, &l_renderPack.metallicDXTDC->m_SRV);
		}
		// any roughness?
		if (l_renderPack.textureCBuffer.useRoughnessTexture)
		{
			DXRenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(3, 1, &l_renderPack.roughnessDXTDC->m_SRV);
		}
		// any ao?
		if (l_renderPack.textureCBuffer.useAOTexture)
		{
			DXRenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(4, 1, &l_renderPack.AODXTDC->m_SRV);
		}

		drawMesh(l_renderPack.indiceSize, l_renderPack.DXMDC);

		DXRenderingSystemComponent::get().m_GPassRenderingDataQueue.pop();
	}
}
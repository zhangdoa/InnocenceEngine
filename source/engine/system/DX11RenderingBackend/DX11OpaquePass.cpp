#include "DX11OpaquePass.h"
#include "DX11RenderingSystemUtilities.h"

#include "../../component/DX11RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingSystemNS;

INNO_PRIVATE_SCOPE DX11OpaquePass
{
	bool initializeShaders();

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//opaquePassCookTorranceVertex.sf" , "", "DX11//opaquePassCookTorrancePixel.sf" };

	EntityID m_entityID;
}

bool DX11OpaquePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX11RenderPassComponent(4, DX11RenderingSystemComponent::get().deferredPassRTVDesc, DX11RenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

bool DX11OpaquePass::initializeShaders()
{
	m_DXSPC = addDX11ShaderProgramComponent(m_entityID);

	m_DXSPC->m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	m_DXSPC->m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	m_DXSPC->m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	m_DXSPC->m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	m_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	m_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	m_DXSPC->m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	m_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	m_DXSPC->m_samplerDesc.MinLOD = 0;
	m_DXSPC->m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	initializeDX11ShaderProgramComponent(m_DXSPC, m_shaderFilePaths);

	return true;
}

bool DX11OpaquePass::update()
{
	// Set Rasterizer State
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetState(
		DX11RenderingSystemComponent::get().m_rasterStateForward);

	// activate shader
	activateDX11ShaderProgramComponent(m_DXSPC);
	updateShaderParameter(ShaderType::VERTEX, 0, DX11RenderingSystemComponent::get().m_cameraCBuffer, &DX11RenderingSystemComponent::get().m_cameraCBufferData);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DX11RenderingSystemComponent::get().m_deviceContext->OMSetRenderTargets(
		(unsigned int)m_DXRPC->m_renderTargetViews.size(),
		&m_DXRPC->m_renderTargetViews[0],
		m_DXRPC->m_depthStencilView);

	// Set the viewport.
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetViewports(
		1,
		&m_DXRPC->m_viewport);

	// Clear the render buffers.
	for (auto i : m_DXRPC->m_renderTargetViews)
	{
		cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), i);
	}
	cleanDSV(m_DXRPC->m_depthStencilView);

	// draw
	while (DX11RenderingSystemComponent::get().m_meshDataQueue.size() > 0)
	{
		auto l_renderPack = DX11RenderingSystemComponent::get().m_meshDataQueue.front();

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

		DX11RenderingSystemComponent::get().m_deviceContext->IASetPrimitiveTopology(l_primitiveTopology);

		updateShaderParameter(ShaderType::VERTEX, 1, DX11RenderingSystemComponent::get().m_meshCBuffer, &l_renderPack.meshCBuffer);
		updateShaderParameter(ShaderType::FRAGMENT, 0, DX11RenderingSystemComponent::get().m_textureCBuffer, &l_renderPack.textureCBuffer);

		// bind to textures
		// any normal?
		if (l_renderPack.textureCBuffer.useNormalTexture)
		{
			DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(0, 1, &l_renderPack.normalDXTDC->m_SRV);
		}
		// any albedo?
		if (l_renderPack.textureCBuffer.useAlbedoTexture)
		{
			DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(1, 1, &l_renderPack.albedoDXTDC->m_SRV);
		}
		// any metallic?
		if (l_renderPack.textureCBuffer.useMetallicTexture)
		{
			DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(2, 1, &l_renderPack.metallicDXTDC->m_SRV);
		}
		// any roughness?
		if (l_renderPack.textureCBuffer.useRoughnessTexture)
		{
			DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(3, 1, &l_renderPack.roughnessDXTDC->m_SRV);
		}
		// any ao?
		if (l_renderPack.textureCBuffer.useAOTexture)
		{
			DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(4, 1, &l_renderPack.AODXTDC->m_SRV);
		}

		drawMesh(l_renderPack.indiceSize, l_renderPack.DXMDC);

		DX11RenderingSystemComponent::get().m_meshDataQueue.pop();
	}

	return true;
}

bool DX11OpaquePass::resize()
{
	return true;
}

bool DX11OpaquePass::reloadShaders()
{
	return true;
}

DX11RenderPassComponent * DX11OpaquePass::getDX11RPC()
{
	return m_DXRPC;
}
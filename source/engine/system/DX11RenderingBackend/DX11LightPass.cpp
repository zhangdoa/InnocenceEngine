#include "DX11LightPass.h"
#include "DX11RenderingSystemUtilities.h"

#include "DX11OpaquePass.h"

#include "../../component/DX11RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingSystemNS;

INNO_PRIVATE_SCOPE DX11LightPass
{
	bool initializeShaders();

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_lightPass_shaderFilePaths = { "DX11//lightPassCookTorranceVertex.sf" , "", "DX11//lightPassCookTorrancePixel.sf" };

	EntityID m_entityID;
}

bool DX11LightPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX11RenderPassComponent(1, DX11RenderingSystemComponent::get().deferredPassRTVDesc, DX11RenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

bool DX11LightPass::initializeShaders()
{
	m_DXSPC = addDX11ShaderProgramComponent(m_entityID);

	DX11CBuffer l_PSCBuffer;

	l_PSCBuffer.m_CBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	l_PSCBuffer.m_CBufferDesc.ByteWidth = sizeof(LPassCBufferData);
	l_PSCBuffer.m_CBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	l_PSCBuffer.m_CBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	l_PSCBuffer.m_CBufferDesc.MiscFlags = 0;
	l_PSCBuffer.m_CBufferDesc.StructureByteStride = 0;

	m_DXSPC->m_PSCBuffers.emplace_back(l_PSCBuffer);

	m_DXSPC->m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	m_DXSPC->m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_DXSPC->m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_DXSPC->m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	m_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	m_DXSPC->m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	m_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	m_DXSPC->m_samplerDesc.MinLOD = 0;
	m_DXSPC->m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	initializeDX11ShaderProgramComponent(m_DXSPC, m_lightPass_shaderFilePaths);

	return true;
}

bool DX11LightPass::update()
{
	// Set Rasterizer State
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetState(
		DX11RenderingSystemComponent::get().m_rasterStateDeferred);

	activateDX11ShaderProgramComponent(m_DXSPC);

	DX11RenderingSystemComponent::get().m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

	auto l_LPassCBufferData = DX11RenderingSystemComponent::get().m_LPassCBufferData;

	updateShaderParameter(ShaderType::FRAGMENT, 0, m_DXSPC->m_PSCBuffers, &l_LPassCBufferData);

	// bind to previous pass render target textures
	DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(0, 1, &DX11OpaquePass::getDX11RPC()->m_DXTDCs[0]->m_SRV);
	DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(1, 1, &DX11OpaquePass::getDX11RPC()->m_DXTDCs[1]->m_SRV);
	DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(2, 1, &DX11OpaquePass::getDX11RPC()->m_DXTDCs[2]->m_SRV);

	// draw
	drawMesh(6, DX11RenderingSystemComponent::get().m_UnitQuadDXMDC);

	return true;
}

bool DX11LightPass::resize()
{
	return true;
}

bool DX11LightPass::reloadShaders()
{
	return true;
}

DX11RenderPassComponent * DX11LightPass::getDX11RPC()
{
	return m_DXRPC;
}
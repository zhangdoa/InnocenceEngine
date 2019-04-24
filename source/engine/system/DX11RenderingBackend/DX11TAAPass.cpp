#include "DX11TAAPass.h"
#include "DX11RenderingSystemUtilities.h"

#include "DX11PreTAAPass.h"
#include "DX11OpaquePass.h"

#include "../../component/DX11RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingSystemNS;

INNO_PRIVATE_SCOPE DX11TAAPass
{
	bool initializeShaders();

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//TAAPassVertex.hlsl" , "", "DX11//TAAPassPixel.hlsl" };

	EntityID m_entityID;

	bool m_isTAAPingPass = true;
}

bool DX11TAAPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX11RenderPassComponent(2, DX11RenderingSystemComponent::get().deferredPassRTVDesc, DX11RenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

bool DX11TAAPass::initializeShaders()
{
	m_DXSPC = addDX11ShaderProgramComponent(m_entityID);

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

	initializeDX11ShaderProgramComponent(m_DXSPC, m_shaderFilePaths);

	return true;
}

bool DX11TAAPass::update()
{
	DX11TextureDataComponent* l_lastFrameDXTDC;
	ID3D11RenderTargetView* l_currentFrameDXRTV;

	if (m_isTAAPingPass)
	{
		l_lastFrameDXTDC = m_DXRPC->m_DXTDCs[1];
		l_currentFrameDXRTV = m_DXRPC->m_renderTargetViews[0];
		m_isTAAPingPass = false;
	}
	else
	{
		l_lastFrameDXTDC = m_DXRPC->m_DXTDCs[0];
		l_currentFrameDXRTV = m_DXRPC->m_renderTargetViews[1];
		m_isTAAPingPass = true;
	}

	// Set the depth stencil state.
	DX11RenderingSystemComponent::get().m_deviceContext->OMSetDepthStencilState(
		m_DXRPC->m_depthStencilState, 1);

	// Set Rasterizer State
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetState(
		DX11RenderingSystemComponent::get().m_rasterStateDeferred);

	activateDX11ShaderProgramComponent(m_DXSPC);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DX11RenderingSystemComponent::get().m_deviceContext->OMSetRenderTargets(
		1,
		&l_currentFrameDXRTV,
		m_DXRPC->m_depthStencilView);

	// Set the viewport.
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetViewports(
		1,
		&m_DXRPC->m_viewport);

	cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), l_currentFrameDXRTV);

	cleanDSV(m_DXRPC->m_depthStencilView);

	// bind to previous pass render target textures
	activateTexture(ShaderType::FRAGMENT, 0, DX11PreTAAPass::getDX11RPC()->m_DXTDCs[0]);
	activateTexture(ShaderType::FRAGMENT, 1, l_lastFrameDXTDC);
	activateTexture(ShaderType::FRAGMENT, 2, DX11OpaquePass::getDX11RPC()->m_DXTDCs[3]);

	// draw
	auto l_MDC = getDX11MeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return true;
}

bool DX11TAAPass::resize()
{
	return true;
}

bool DX11TAAPass::reloadShaders()
{
	return true;
}

DX11RenderPassComponent * DX11TAAPass::getDX11RPC()
{
	return m_DXRPC;
}

DX11TextureDataComponent* DX11TAAPass::getResult()
{
	if (m_isTAAPingPass)
	{
		return m_DXRPC->m_DXTDCs[1];
	}
	else
	{
		return m_DXRPC->m_DXTDCs[0];
	}
}
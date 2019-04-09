#include "DX11FinalBlendPass.h"
#include "DX11RenderingSystemUtilities.h"

#include "DX11LightPass.h"

#include "../../component/DX11RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingSystemNS;

INNO_PRIVATE_SCOPE DX11FinalBlendPass
{
	EntityID m_entityID;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_finalPass_shaderFilePaths = { "DX11//finalBlendPassVertex.sf" , "", "DX11//finalBlendPassPixel.sf" };
}

bool DX11FinalBlendPass::initialize()
{
	m_entityID = InnoMath::createEntityID();
	m_DXSPC = addDX11ShaderProgramComponent(m_entityID);

	// Create a texture sampler state description.
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

	initializeDX11ShaderProgramComponent(m_DXSPC, m_finalPass_shaderFilePaths);

	return true;
}

bool DX11FinalBlendPass::update()
{
	// Set Rasterizer State
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetState(
		DX11RenderingSystemComponent::get().m_rasterStateDeferred);

	activateDX11ShaderProgramComponent(m_DXSPC);

	DX11RenderingSystemComponent::get().m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DX11RenderingSystemComponent::get().m_deviceContext->OMSetRenderTargets(
		1,
		&DX11RenderingSystemComponent::get().m_renderTargetView,
		DX11RenderingSystemComponent::get().m_depthStencilView);

	// Set the viewport.
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetViewports(
		1,
		&DX11RenderingSystemComponent::get().m_viewport);

	// Clear the render buffers.
	cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), DX11RenderingSystemComponent::get().m_renderTargetView);
	cleanDSV(DX11RenderingSystemComponent::get().m_depthStencilView);

	// bind to previous pass render target textures
	DX11RenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(0, 1, &DX11LightPass::getDX11RPC()->m_DXTDCs[0]->m_SRV);

	// draw
	drawMesh(6, DX11RenderingSystemComponent::get().m_UnitQuadDXMDC);

	return true;
}

bool DX11FinalBlendPass::resize()
{
	return true;
}

bool DX11FinalBlendPass::reloadShaders()
{
	return true;
}
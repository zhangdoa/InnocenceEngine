#include "DXRenderingSystemUtilities.h"
#include "DXFinalRenderingPassUtilities.h"
#include "../component/DXFinalRenderPassComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/DXRenderingSystemComponent.h"
#include "../component/PhysicsSystemComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/DXGeometryRenderPassComponent.h"
#include "../component/DXLightRenderPassComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DXRenderingSystemNS;

INNO_PRIVATE_SCOPE DXFinalRenderingPassUtilities
{
	EntityID m_entityID;
}

void DXFinalRenderingPassUtilities::initialize()
{
	m_entityID = InnoMath::createEntityID();
	DXFinalRenderPassComponent::get().m_DXSPC = g_pCoreSystem->getMemorySystem()->spawn<DXShaderProgramComponent>();

	// Create a texture sampler state description.
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.MinLOD = 0;
	DXFinalRenderPassComponent::get().m_DXSPC->m_samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	initializeDXShaderProgramComponent(DXFinalRenderPassComponent::get().m_DXSPC, DXFinalRenderPassComponent::get().m_finalPass_shaderFilePaths);
}

void DXFinalRenderingPassUtilities::update()
{
	// Set Rasterizer State
	DXRenderingSystemComponent::get().m_deviceContext->RSSetState(
		DXRenderingSystemComponent::get().m_rasterStateDeferred);

	activateDXShaderProgramComponent(DXFinalRenderPassComponent::get().m_DXSPC);

	DXRenderingSystemComponent::get().m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set the render buffers to be the render target.
	// Bind the render target view array and depth stencil buffer to the output render pipeline.
	DXRenderingSystemComponent::get().m_deviceContext->OMSetRenderTargets(
		1,
		&DXRenderingSystemComponent::get().m_renderTargetView,
		DXRenderingSystemComponent::get().m_depthStencilView);

	// Set the viewport.
	DXRenderingSystemComponent::get().m_deviceContext->RSSetViewports(
		1,
		&DXRenderingSystemComponent::get().m_viewport);

	// Clear the render buffers.
	cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), DXRenderingSystemComponent::get().m_renderTargetView);
	cleanDSV(DXRenderingSystemComponent::get().m_depthStencilView);

	// bind to previous pass render target textures
	DXRenderingSystemComponent::get().m_deviceContext->PSSetShaderResources(0, 1, &DXLightRenderPassComponent::get().m_DXRPC->m_DXTDCs[0]->m_SRV);

	// draw
	drawMesh(6, DXRenderingSystemComponent::get().m_UnitQuadDXMDC);
}

bool DXFinalRenderingPassUtilities::resize()
{
	return true;
}

bool DXFinalRenderingPassUtilities::reloadFinalPassShaders()
{
	return true;
}
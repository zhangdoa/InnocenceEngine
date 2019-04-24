#include "DX11FinalBlendPass.h"
#include "DX11RenderingSystemUtilities.h"

#include "DX11TAAPass.h"

#include "../../component/DX11RenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingSystemNS;

INNO_PRIVATE_SCOPE DX11FinalBlendPass
{
	EntityID m_entityID;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//finalBlendPassVertex.hlsl" , "", "DX11//finalBlendPassPixel.hlsl" };
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

	initializeDX11ShaderProgramComponent(m_DXSPC, m_shaderFilePaths);

	return true;
}

bool DX11FinalBlendPass::update()
{
	// Set the depth stencil state.
	DX11RenderingSystemComponent::get().m_deviceContext->OMSetDepthStencilState(
		DX11RenderingSystemComponent::get().m_defaultDepthStencilState, 1);

	// Set Rasterizer State
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetState(
		DX11RenderingSystemComponent::get().m_rasterStateDeferred);

	activateDX11ShaderProgramComponent(m_DXSPC);

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
	auto l_canvasDXTDC = DX11TAAPass::getResult();
	bindTextureForRead(ShaderType::FRAGMENT, 0, l_canvasDXTDC);

	// draw
	auto l_MDC = getDX11MeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

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
#include "DX11LightPass.h"
#include "DX11RenderingBackendUtilities.h"

#include "DX11OpaquePass.h"
#include "DX11LightCullingPass.h"

#include "../../../Component/DX11RenderingBackendComponent.h"
#include "../../../Component/RenderingFrontendComponent.h"

#include "../../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingBackendNS;

INNO_PRIVATE_SCOPE DX11LightPass
{
	bool initializeShaders();

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//lightPassVertex.hlsl/", "", "", "", "DX11//lightPassPixel.hlsl/" };

	EntityID m_entityID;
}

bool DX11LightPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX11RenderPassComponent(m_entityID, "LightPassDXRPC\\");

	m_DXRPC->m_renderPassDesc = DX11RenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_DXRPC->m_renderPassDesc.RTNumber = 1;
	m_DXRPC->m_renderPassDesc.useDepthAttachment = true;
	m_DXRPC->m_renderPassDesc.useStencilAttachment = true;

	// Set up the description of the depth stencil state.
	m_DXRPC->m_depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	m_DXRPC->m_depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	m_DXRPC->m_depthStencilDesc.StencilReadMask = 0xFF;
	m_DXRPC->m_depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// Stencil operations if pixel is back-facing.
	m_DXRPC->m_depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// Setup the raster description.
	m_DXRPC->m_rasterizerDesc.AntialiasedLineEnable = true;
	m_DXRPC->m_rasterizerDesc.CullMode = D3D11_CULL_NONE;
	m_DXRPC->m_rasterizerDesc.DepthBias = 0;
	m_DXRPC->m_rasterizerDesc.DepthBiasClamp = 0.0f;
	m_DXRPC->m_rasterizerDesc.DepthClipEnable = true;
	m_DXRPC->m_rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	m_DXRPC->m_rasterizerDesc.FrontCounterClockwise = false;
	m_DXRPC->m_rasterizerDesc.MultisampleEnable = true;
	m_DXRPC->m_rasterizerDesc.ScissorEnable = false;
	m_DXRPC->m_rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	initializeShaders();
	initializeDX11RenderPassComponent(m_DXRPC);

	return true;
}

bool DX11LightPass::initializeShaders()
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

bool DX11LightPass::update()
{
	activateShader(m_DXSPC);

	DX11RenderingBackendComponent::get().m_deviceContext->OMSetDepthStencilState(m_DXRPC->m_depthStencilState, 0x01);

	DX11RenderingBackendComponent::get().m_deviceContext->RSSetState(m_DXRPC->m_rasterizerState);

	DX11RenderingBackendComponent::get().m_deviceContext->OMSetRenderTargets((unsigned int)m_DXRPC->m_RTVs.size(), &m_DXRPC->m_RTVs[0], DX11OpaquePass::getDX11RPC()->m_DSV);

	DX11RenderingBackendComponent::get().m_deviceContext->RSSetViewports(1, &m_DXRPC->m_viewport);

	for (auto i : m_DXRPC->m_RTVs)
	{
		cleanRTV(vec4(0.0f, 0.0f, 0.0f, 0.0f), i);
	}

	bindConstantBuffer(ShaderType::FRAGMENT, 0, DX11RenderingBackendComponent::get().m_cameraConstantBuffer);
	bindConstantBuffer(ShaderType::FRAGMENT, 1, DX11RenderingBackendComponent::get().m_sunConstantBuffer);
	bindConstantBuffer(ShaderType::FRAGMENT, 2, DX11RenderingBackendComponent::get().m_pointLightConstantBuffer);
	bindConstantBuffer(ShaderType::FRAGMENT, 3, DX11RenderingBackendComponent::get().m_sphereLightConstantBuffer);
	bindConstantBuffer(ShaderType::FRAGMENT, 4, DX11RenderingBackendComponent::get().m_skyConstantBuffer);

	// bind to previous pass render target textures
	bindTextureForRead(ShaderType::FRAGMENT, 0, DX11OpaquePass::getDX11RPC()->m_DXTDCs[0]);
	bindTextureForRead(ShaderType::FRAGMENT, 1, DX11OpaquePass::getDX11RPC()->m_DXTDCs[1]);
	bindTextureForRead(ShaderType::FRAGMENT, 2, DX11OpaquePass::getDX11RPC()->m_DXTDCs[2]);

	bindStructuredBufferForRead(ShaderType::FRAGMENT, 3, DX11RenderingBackendComponent::get().m_lightIndexListStructuredBuffer);
	bindTextureForRead(ShaderType::FRAGMENT, 4, DX11LightCullingPass::getLightGrid());

	// draw
	auto l_MDC = getDX11MeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	unbindTextureForRead(ShaderType::FRAGMENT, 0);
	unbindTextureForRead(ShaderType::FRAGMENT, 1);
	unbindTextureForRead(ShaderType::FRAGMENT, 2);
	unbindStructuredBufferForRead(ShaderType::FRAGMENT, 3);
	unbindTextureForRead(ShaderType::FRAGMENT, 4);

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
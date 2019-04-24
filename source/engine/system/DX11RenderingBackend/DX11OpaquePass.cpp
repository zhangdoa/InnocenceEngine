#include "DX11OpaquePass.h"
#include "DX11RenderingSystemUtilities.h"

#include "../../component/DX11RenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX11RenderingSystemNS;

INNO_PRIVATE_SCOPE DX11OpaquePass
{
	bool initializeShaders();

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//opaquePassVertex.hlsl" , "", "DX11//opaquePassPixel.hlsl" };

	EntityID m_entityID;
}

bool DX11OpaquePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX11RenderPassComponent(4, DX11RenderingSystemComponent::get().deferredPassRTVDesc, DX11RenderingSystemComponent::get().deferredPassTextureDesc);

	// Set up the description of the stencil state.
	m_DXRPC->m_depthStencilDesc.DepthEnable = true;
	m_DXRPC->m_depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	m_DXRPC->m_depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	m_DXRPC->m_depthStencilDesc.StencilEnable = true;
	m_DXRPC->m_depthStencilDesc.StencilReadMask = 0xFF;
	m_DXRPC->m_depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	m_DXRPC->m_depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	auto result = DX11RenderingSystemComponent::get().m_device->CreateDepthStencilState(
		&m_DXRPC->m_depthStencilDesc,
		&m_DXRPC->m_depthStencilState);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DX11RenderingSystem: can't create the depth stencil state for opaque pass!");
		return false;
	}

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
	// Set the depth stencil state.
	DX11RenderingSystemComponent::get().m_deviceContext->OMSetDepthStencilState(
		m_DXRPC->m_depthStencilState, 0x01);

	// Set Rasterizer State
	DX11RenderingSystemComponent::get().m_deviceContext->RSSetState(
		DX11RenderingSystemComponent::get().m_rasterStateForward);

	// activate shader
	activateDX11ShaderProgramComponent(m_DXSPC);
	bindConstantBuffer(ShaderType::VERTEX, 0, DX11RenderingSystemComponent::get().m_cameraConstantBuffer);

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
	while (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.size() > 0)
	{
		GeometryPassGPUData l_geometryPassGPUData = {};

		if (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.tryPop(l_geometryPassGPUData))
		{
			// bind to textures
			// any normal?
			if (l_geometryPassGPUData.materialGPUData.useNormalTexture)
			{
				activateTexture(ShaderType::FRAGMENT, 0, reinterpret_cast<DX11TextureDataComponent*>(l_geometryPassGPUData.normalTDC));
			}
			// any albedo?
			if (l_geometryPassGPUData.materialGPUData.useAlbedoTexture)
			{
				activateTexture(ShaderType::FRAGMENT, 1, reinterpret_cast<DX11TextureDataComponent*>(l_geometryPassGPUData.albedoTDC));
			}
			// any metallic?
			if (l_geometryPassGPUData.materialGPUData.useMetallicTexture)
			{
				activateTexture(ShaderType::FRAGMENT, 2, reinterpret_cast<DX11TextureDataComponent*>(l_geometryPassGPUData.metallicTDC));
			}
			// any roughness?
			if (l_geometryPassGPUData.materialGPUData.useRoughnessTexture)
			{
				activateTexture(ShaderType::FRAGMENT, 3, reinterpret_cast<DX11TextureDataComponent*>(l_geometryPassGPUData.roughnessTDC));
			}
			// any ao?
			if (l_geometryPassGPUData.materialGPUData.useAOTexture)
			{
				activateTexture(ShaderType::FRAGMENT, 4, reinterpret_cast<DX11TextureDataComponent*>(l_geometryPassGPUData.AOTDC));
			}
			updateConstantBuffer(DX11RenderingSystemComponent::get().m_meshConstantBuffer, &l_geometryPassGPUData.meshGPUData);
			updateConstantBuffer(DX11RenderingSystemComponent::get().m_materialConstantBuffer, &l_geometryPassGPUData.materialGPUData);

			bindConstantBuffer(ShaderType::VERTEX, 1, DX11RenderingSystemComponent::get().m_meshConstantBuffer);
			bindConstantBuffer(ShaderType::FRAGMENT, 0, DX11RenderingSystemComponent::get().m_materialConstantBuffer);

			drawMesh(reinterpret_cast<DX11MeshDataComponent*>(l_geometryPassGPUData.MDC));
		}
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
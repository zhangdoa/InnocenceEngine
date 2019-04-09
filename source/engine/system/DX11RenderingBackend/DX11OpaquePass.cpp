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

	m_DXSPC->m_VSCBuffers.reserve(2);

	DX11CBuffer l_VSCameraCBuffer;
	l_VSCameraCBuffer.m_CBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	l_VSCameraCBuffer.m_CBufferDesc.ByteWidth = sizeof(GPassCameraCBufferData);
	l_VSCameraCBuffer.m_CBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	l_VSCameraCBuffer.m_CBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	l_VSCameraCBuffer.m_CBufferDesc.MiscFlags = 0;
	l_VSCameraCBuffer.m_CBufferDesc.StructureByteStride = 0;
	m_DXSPC->m_VSCBuffers.emplace_back(l_VSCameraCBuffer);

	DX11CBuffer l_VSMeshCBuffer;
	l_VSMeshCBuffer.m_CBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	l_VSMeshCBuffer.m_CBufferDesc.ByteWidth = sizeof(GPassMeshCBufferData);
	l_VSMeshCBuffer.m_CBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	l_VSMeshCBuffer.m_CBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	l_VSMeshCBuffer.m_CBufferDesc.MiscFlags = 0;
	l_VSMeshCBuffer.m_CBufferDesc.StructureByteStride = 0;
	m_DXSPC->m_VSCBuffers.emplace_back(l_VSMeshCBuffer);

	m_DXSPC->m_PSCBuffers.reserve(1);

	DX11CBuffer l_PSCBuffer;
	l_PSCBuffer.m_CBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	l_PSCBuffer.m_CBufferDesc.ByteWidth = sizeof(GPassTextureCBufferData);
	l_PSCBuffer.m_CBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	l_PSCBuffer.m_CBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	l_PSCBuffer.m_CBufferDesc.MiscFlags = 0;
	l_PSCBuffer.m_CBufferDesc.StructureByteStride = 0;
	m_DXSPC->m_PSCBuffers.emplace_back(l_VSMeshCBuffer);

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

	activateDX11ShaderProgramComponent(m_DXSPC);

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
	while (DX11RenderingSystemComponent::get().m_GPassMeshDataQueue.size() > 0)
	{
		auto l_renderPack = DX11RenderingSystemComponent::get().m_GPassMeshDataQueue.front();

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

		updateShaderParameter(ShaderType::VERTEX, 0, m_DXSPC->m_VSCBuffers, &DX11RenderingSystemComponent::get().m_GPassCameraCBufferData);
		updateShaderParameter(ShaderType::VERTEX, 1, m_DXSPC->m_VSCBuffers, &l_renderPack.meshCBuffer);
		updateShaderParameter(ShaderType::FRAGMENT, 0, m_DXSPC->m_PSCBuffers, &l_renderPack.textureCBuffer);

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

		DX11RenderingSystemComponent::get().m_GPassMeshDataQueue.pop();
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
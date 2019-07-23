#include "DX11FinalBlendPass.h"
#include "DX11RenderingBackendUtilities.h"

#include "DX11MotionBlurPass.h"
#include "DX11LightCullingPass.h"

#include "../../Component/DX11RenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace DX11RenderingBackendNS;

namespace DX11FinalBlendPass
{
	EntityID m_entityID;

	DX11RenderPassComponent* m_DXRPC;
	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//finalBlendPassVertex.hlsl/" , "", "", "", "DX11//finalBlendPassPixel.hlsl/" };

	std::function<void()> f_toggleVisualizeLightCulling;
	bool m_visualizeLightCulling = false;
}

bool DX11FinalBlendPass::initialize()
{
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

	f_toggleVisualizeLightCulling = [&]() {
		m_visualizeLightCulling = !m_visualizeLightCulling;
	};

	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_T, ButtonStatus::PRESSED }, &f_toggleVisualizeLightCulling);

	auto l_imageCount = 1;

	m_DXRPC = addDX11RenderPassComponent(m_entityID, "FinalBlendPassDXRPC\\");

	m_DXRPC->m_renderPassDesc = DX11RenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_DXRPC->m_renderPassDesc.RTNumber = l_imageCount;
	m_DXRPC->m_renderPassDesc.useMultipleFramebuffers = false;
	m_DXRPC->m_renderPassDesc.useDepthAttachment = false;
	m_DXRPC->m_renderPassDesc.useStencilAttachment = false;

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

	// initialize manually
	bool l_result = true;
	l_result &= reserveRenderTargets(m_DXRPC);

	for (size_t i = 0; i < DX11RenderingBackendComponent::get().m_swapChainTextures.size(); i++)
	{
		m_DXRPC->m_DXTDCs[i]->m_texture = DX11RenderingBackendComponent::get().m_swapChainTextures[i];
		DX11RenderingBackendComponent::get().m_swapChainTextures[i]->GetDesc(&m_DXRPC->m_DXTDCs[i]->m_DX11TextureDataDesc);
	}

	l_result &= createRTV(m_DXRPC);
	l_result &= setupPipeline(m_DXRPC);

	return true;
}

bool DX11FinalBlendPass::update()
{
	activateShader(m_DXSPC);

	activateRenderPass(m_DXRPC);

	// bind to previous pass render target textures
	auto l_canvasDXTDC = DX11MotionBlurPass::getDX11RPC()->m_DXTDCs[0];
	if (m_visualizeLightCulling)
	{
		l_canvasDXTDC = DX11LightCullingPass::getHeatMap();
	}

	bindTextureForRead(ShaderType::FRAGMENT, 0, l_canvasDXTDC);

	// draw
	auto l_MDC = getDX11MeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	unbindTextureForRead(ShaderType::FRAGMENT, 0);

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

DX11RenderPassComponent * DX11FinalBlendPass::getDX11RPC()
{
	return m_DXRPC;
}
#include "DX11FinalBlendPass.h"
#include "DX11RenderingBackendUtilities.h"

#include "DX11MotionBlurPass.h"
#include "DX11LightCullingPass.h"

#include "../../Component/DX11RenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace DX11RenderingBackendNS;

INNO_PRIVATE_SCOPE DX11FinalBlendPass
{
	EntityID m_entityID;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//finalBlendPassVertex.hlsl/" , "", "", "", "DX11//finalBlendPassPixel.hlsl/" };

	std::function<void()> f_toggleVisualizeLightCulling;
	bool m_visualizeLightCulling = false;
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

	f_toggleVisualizeLightCulling = [&]() {
		m_visualizeLightCulling = !m_visualizeLightCulling;
	};

	g_pModuleManager->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_T, ButtonStatus::PRESSED }, &f_toggleVisualizeLightCulling);

	return true;
}

bool DX11FinalBlendPass::update()
{
	activateShader(m_DXSPC);

	activateRenderPass(DX11RenderingBackendComponent::get().m_swapChainDXRPC);

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
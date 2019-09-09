#include "DX11MotionBlurPass.h"
#include "DX11RenderingBackendUtilities.h"

#include "DX11OpaquePass.h"
#include "DX11PostTAAPass.h"

#include "../../Component/DX11RenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace DX11RenderingBackendNS;

namespace DX11MotionBlurPass
{
	bool initializeShaders();

	DX11RenderPassComponent* m_DXRPC;

	DX11ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX11//motionBlurPassVertex.hlsl/", "", "", "", "DX11//motionBlurPassPixel.hlsl/" };

	EntityID m_EntityID;
}

bool DX11MotionBlurPass::initialize()
{
	m_EntityID = InnoMath::createEntityID();

	m_DXRPC = addDX11RenderPassComponent(m_EntityID, "MotionBlurPassDXRPC\\");

	m_DXRPC->m_renderPassDesc = DX11RenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_DXRPC->m_renderPassDesc.RTNumber = 1;
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

	initializeShaders();
	initializeDX11RenderPassComponent(m_DXRPC);

	return true;
}

bool DX11MotionBlurPass::initializeShaders()
{
	m_DXSPC = addDX11ShaderProgramComponent(m_EntityID);

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

bool DX11MotionBlurPass::update()
{
	activateShader(m_DXSPC);

	activateRenderPass(m_DXRPC);

	// bind to previous pass render target textures
	bindTextureForRead(ShaderType::FRAGMENT, 0, DX11OpaquePass::getDX11RPC()->m_DXTDCs[3]);
	bindTextureForRead(ShaderType::FRAGMENT, 1, DX11PostTAAPass::getDX11RPC()->m_DXTDCs[0]);

	// draw
	auto l_MDC = getDX11MeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	unbindTextureForRead(ShaderType::FRAGMENT, 0);
	unbindTextureForRead(ShaderType::FRAGMENT, 1);

	return true;
}

bool DX11MotionBlurPass::resize()
{
	return true;
}

bool DX11MotionBlurPass::reloadShaders()
{
	return true;
}

DX11RenderPassComponent * DX11MotionBlurPass::getDX11RPC()
{
	return m_DXRPC;
}
#include "DX12LightPass.h"
#include "DX12RenderingBackendUtilities.h"

#include "DX12OpaquePass.h"

#include "../../Component/DX12RenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace DX12RenderingBackendNS;

INNO_PRIVATE_SCOPE DX12LightPass
{
	bool initializeShaders();

	DX12RenderPassComponent* m_DXRPC;

	DX12ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX12//lightPassVertex.hlsl/", "", "", "", "DX12//lightPassPixel.hlsl/" };

	EntityID m_entityID;
}

bool DX12LightPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX12RenderPassComponent(m_entityID, "LightPassDXRPC\\");

	m_DXRPC->m_renderPassDesc = DX12RenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_DXRPC->m_renderPassDesc.RTNumber = 1;
	m_DXRPC->m_renderPassDesc.useDepthAttachment = true;
	m_DXRPC->m_renderPassDesc.useStencilAttachment = true;

	// Setup root signature.
	CD3DX12_ROOT_PARAMETER1 l_rootParams[7];
	l_rootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	l_rootParams[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	l_rootParams[2].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	l_rootParams[3].InitAsConstantBufferView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	l_rootParams[4].InitAsConstantBufferView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_DESCRIPTOR_RANGE1 l_opaquePassRT0DescRange;
	l_opaquePassRT0DescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

	CD3DX12_DESCRIPTOR_RANGE1 l_samplerDescRange;
	l_samplerDescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	l_rootParams[5].InitAsDescriptorTable(1, &l_opaquePassRT0DescRange, D3D12_SHADER_VISIBILITY_PIXEL);
	l_rootParams[6].InitAsDescriptorTable(1, &l_samplerDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC l_rootSigDesc(sizeof(l_rootParams) / sizeof(l_rootParams[0]), l_rootParams);
	m_DXRPC->m_rootSignatureDesc = l_rootSigDesc;
	m_DXRPC->m_rootSignatureDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Setup the description of the depth stencil state.
	m_DXRPC->m_depthStencilDesc.DepthEnable = true;
	m_DXRPC->m_depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	m_DXRPC->m_depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	m_DXRPC->m_depthStencilDesc.StencilEnable = true;
	m_DXRPC->m_depthStencilDesc.StencilReadMask = 0xFF;
	m_DXRPC->m_depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	m_DXRPC->m_depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Stencil operations if pixel is back-facing.
	m_DXRPC->m_depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	m_DXRPC->m_depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	m_DXRPC->m_rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	m_DXRPC->m_blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// Setup the viewport.
	m_DXRPC->m_viewport.Width = (float)m_DXRPC->m_renderPassDesc.RTDesc.width;
	m_DXRPC->m_viewport.Height = (float)m_DXRPC->m_renderPassDesc.RTDesc.height;
	m_DXRPC->m_viewport.MinDepth = 0.0f;
	m_DXRPC->m_viewport.MaxDepth = 1.0f;
	m_DXRPC->m_viewport.TopLeftX = 0.0f;
	m_DXRPC->m_viewport.TopLeftY = 0.0f;

	// Setup the scissor rect.
	m_DXRPC->m_scissor.left = 0;
	m_DXRPC->m_scissor.top = 0;
	m_DXRPC->m_scissor.right = (unsigned long)m_DXRPC->m_viewport.Width;
	m_DXRPC->m_scissor.bottom = (unsigned long)m_DXRPC->m_viewport.Height;

	// Setup PSO.
	m_DXRPC->m_PSODesc.SampleMask = UINT_MAX;
	m_DXRPC->m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_DXRPC->m_PSODesc.SampleDesc.Count = 1;

	initializeShaders();

	initializeDX12RenderPassComponent(m_DXRPC, m_DXSPC);

	return true;
}

bool DX12LightPass::initializeShaders()
{
	m_DXSPC = addDX12ShaderProgramComponent(m_entityID);

	m_DXSPC->m_samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	m_DXSPC->m_samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_DXSPC->m_samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_DXSPC->m_samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_DXSPC->m_samplerDesc.MipLODBias = 0.0f;
	m_DXSPC->m_samplerDesc.MaxAnisotropy = 1;
	m_DXSPC->m_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	m_DXSPC->m_samplerDesc.BorderColor[0] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[1] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[2] = 0;
	m_DXSPC->m_samplerDesc.BorderColor[3] = 0;
	m_DXSPC->m_samplerDesc.MinLOD = 0;
	m_DXSPC->m_samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	initializeDX12ShaderProgramComponent(m_DXSPC, m_shaderFilePaths);

	return true;
}

bool DX12LightPass::update()
{
	auto l_MDC = getDX12MeshDataComponent(MeshShapeType::QUAD);

	recordCommandBegin(m_DXRPC, 0);

	recordActivateRenderPass(m_DXRPC, 0);

	recordBindRTForRead(m_DXRPC, 0, DX12OpaquePass::getDX12RPC()->m_renderTargets[0]);
	recordBindRTForRead(m_DXRPC, 0, DX12OpaquePass::getDX12RPC()->m_renderTargets[1]);
	recordBindRTForRead(m_DXRPC, 0, DX12OpaquePass::getDX12RPC()->m_renderTargets[2]);

	ID3D12DescriptorHeap* l_heaps[] = { DX12RenderingBackendComponent::get().m_CSUHeap, DX12RenderingBackendComponent::get().m_samplerHeap };

	recordBindDescHeaps(m_DXRPC, 0, 2, l_heaps);

	recordBindCBV(m_DXRPC, 0, 0, DX12RenderingBackendComponent::get().m_cameraConstantBuffer, 0);
	recordBindCBV(m_DXRPC, 0, 1, DX12RenderingBackendComponent::get().m_sunConstantBuffer, 0);
	recordBindCBV(m_DXRPC, 0, 2, DX12RenderingBackendComponent::get().m_pointLightConstantBuffer, 0);
	recordBindCBV(m_DXRPC, 0, 3, DX12RenderingBackendComponent::get().m_sphereLightConstantBuffer, 0);
	recordBindCBV(m_DXRPC, 0, 4, DX12RenderingBackendComponent::get().m_skyConstantBuffer, 0);

	recordBindSRVDescTable(m_DXRPC, 0, 5, DX12OpaquePass::getDX12RPC()->m_SRVs[0]);
	recordBindSamplerDescTable(m_DXRPC, 0, 6, m_DXSPC);

	recordDrawCall(m_DXRPC, 0, l_MDC);

	recordBindRTForWrite(m_DXRPC, 0, DX12OpaquePass::getDX12RPC()->m_renderTargets[0]);
	recordBindRTForWrite(m_DXRPC, 0, DX12OpaquePass::getDX12RPC()->m_renderTargets[1]);
	recordBindRTForWrite(m_DXRPC, 0, DX12OpaquePass::getDX12RPC()->m_renderTargets[2]);

	recordCommandEnd(m_DXRPC, 0);

	return true;
}

bool DX12LightPass::render()
{
	executeCommandList(m_DXRPC, 0);

	return true;
}

bool DX12LightPass::terminate()
{
	destroyDX12RenderPassComponent(m_DXRPC);
	return true;
}

bool DX12LightPass::resize()
{
	return true;
}

bool DX12LightPass::reloadShaders()
{
	return true;
}

DX12RenderPassComponent * DX12LightPass::getDX12RPC()
{
	return m_DXRPC;
}
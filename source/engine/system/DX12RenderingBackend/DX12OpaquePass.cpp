#include "DX12OpaquePass.h"
#include "DX12RenderingSystemUtilities.h"

#include "../../component/DX12RenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace DX12RenderingSystemNS;

INNO_PRIVATE_SCOPE DX12OpaquePass
{
	bool initializeShaders();

	DX12RenderPassComponent* m_DXRPC;

	DX12ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX12//opaquePassVertex.hlsl" , "", "DX12//opaquePassPixel.hlsl" };

	EntityID m_entityID;
}

bool DX12OpaquePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_DXRPC = addDX12RenderPassComponent(m_entityID);

	m_DXRPC->m_renderPassDesc = DX12RenderingSystemComponent::get().m_deferredRenderPassDesc;
	m_DXRPC->m_renderPassDesc.RTNumber = 4;
	m_DXRPC->m_renderPassDesc.useMultipleFramebuffers = false;

	m_DXRPC->m_RTVHeapDesc.NumDescriptors = 4;
	m_DXRPC->m_RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	m_DXRPC->m_RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	CD3DX12_DESCRIPTOR_RANGE1 l_descRange[3];
	l_descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 3, 0);
	l_descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);
	l_descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	CD3DX12_ROOT_PARAMETER1 l_rootParams[3];
	l_rootParams[0].InitAsDescriptorTable(1, &l_descRange[0]);
	l_rootParams[1].InitAsDescriptorTable(1, &l_descRange[1]);
	l_rootParams[2].InitAsDescriptorTable(1, &l_descRange[2]);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC l_rootSigDesc(3, l_rootParams);
	m_DXRPC->m_rootSignatureDesc = l_rootSigDesc;
	m_DXRPC->m_rootSignatureDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	m_DXRPC->m_rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	m_DXRPC->m_blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// Set up the depth stencil view description.
	m_DXRPC->m_depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	m_DXRPC->m_depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_DXRPC->m_depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Setup the viewport for rendering.
	m_DXRPC->m_viewport.Width = (float)m_DXRPC->m_renderPassDesc.RTDesc.width;
	m_DXRPC->m_viewport.Height = (float)m_DXRPC->m_renderPassDesc.RTDesc.height;
	m_DXRPC->m_viewport.MinDepth = 0.0f;
	m_DXRPC->m_viewport.MaxDepth = 1.0f;
	m_DXRPC->m_viewport.TopLeftX = 0.0f;
	m_DXRPC->m_viewport.TopLeftY = 0.0f;

	// Setup the scissor rect
	m_DXRPC->m_scissor.left = 0;
	m_DXRPC->m_scissor.top = 0;
	m_DXRPC->m_scissor.right = (unsigned long)m_DXRPC->m_viewport.Width;
	m_DXRPC->m_scissor.bottom = (unsigned long)m_DXRPC->m_viewport.Height;

	// Describe and create the graphics pipeline state object (PSO).
	m_DXRPC->m_PSODesc.RasterizerState = m_DXRPC->m_rasterizerDesc;
	m_DXRPC->m_PSODesc.BlendState = m_DXRPC->m_blendDesc;
	m_DXRPC->m_PSODesc.DepthStencilState.DepthEnable = false;
	m_DXRPC->m_PSODesc.DepthStencilState.StencilEnable = false;
	m_DXRPC->m_PSODesc.SampleMask = UINT_MAX;
	m_DXRPC->m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_DXRPC->m_PSODesc.SampleDesc.Count = 1;

	initializeShaders();

	initializeDX12RenderPassComponent(m_DXRPC, m_DXSPC);

	return true;
}

bool DX12OpaquePass::initializeShaders()
{
	m_DXSPC = addDX12ShaderProgramComponent(m_entityID);

	initializeDX12ShaderProgramComponent(m_DXSPC, m_shaderFilePaths);

	return true;
}

bool DX12OpaquePass::update()
{
	return true;
}

bool DX12OpaquePass::resize()
{
	return true;
}

bool DX12OpaquePass::reloadShaders()
{
	return true;
}

DX12RenderPassComponent * DX12OpaquePass::getDX12RPC()
{
	return m_DXRPC;
}
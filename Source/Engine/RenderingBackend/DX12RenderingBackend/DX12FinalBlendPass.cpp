#include "DX12FinalBlendPass.h"
#include "DX12RenderingBackendUtilities.h"

#include "DX12LightPass.h"

#include "../../Component/DX12RenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace DX12RenderingBackendNS;

namespace DX12FinalBlendPass
{
	bool initializeShaders();

	DX12RenderPassComponent* m_DXRPC;

	DX12ShaderProgramComponent* m_DXSPC;

	ShaderFilePaths m_shaderFilePaths = { "DX12//finalBlendPassVertex.hlsl/", "", "", "", "DX12//finalBlendPassPixel.hlsl/" };

	EntityID m_entityID;

	uint32_t m_imageCount = 2;
}

bool DX12FinalBlendPass::setup()
{
	m_entityID = InnoMath::createEntityID();

	m_DXSPC = addDX12ShaderProgramComponent(m_entityID);

	initializeShaders();

	m_DXRPC = addDX12RenderPassComponent(m_entityID, "SwapChainDXRPC\\");

	m_DXRPC->m_renderPassDesc = DX12RenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_DXRPC->m_renderPassDesc.RTNumber = m_imageCount;
	m_DXRPC->m_renderPassDesc.useMultipleFramebuffers = true;
	m_DXRPC->m_renderPassDesc.useDepthAttachment = true;
	m_DXRPC->m_renderPassDesc.useStencilAttachment = true;

	// initialize manually
	bool l_result = true;
	l_result &= reserveRenderTargets(m_DXRPC);

	// use local command queue for swap chain
	l_result = createCommandQueue(m_DXRPC);
	l_result = createCommandAllocators(m_DXRPC);
	l_result = createCommandLists(m_DXRPC);

	return true;
}

bool DX12FinalBlendPass::initialize()
{
	for (size_t i = 0; i < m_imageCount; i++)
	{
		m_DXRPC->m_renderTargets[i]->m_texture = DX12RenderingBackendComponent::get().m_swapChainImages[i];
		m_DXRPC->m_renderTargets[i]->m_DX12TextureDataDesc = m_DXRPC->m_renderTargets[i]->m_texture->GetDesc();
	}

	m_DXRPC->m_depthStencilTarget = addDX12TextureDataComponent();
	m_DXRPC->m_depthStencilTarget->m_textureDataDesc = DX12RenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc;
	m_DXRPC->m_depthStencilTarget->m_textureDataDesc.usageType = TextureUsageType::DEPTH_STENCIL_ATTACHMENT;
	m_DXRPC->m_depthStencilTarget->m_textureData = { nullptr };

	initializeDX12TextureDataComponent(m_DXRPC->m_depthStencilTarget);

	bool l_result = true;

	l_result &= createRTVDescriptorHeap(m_DXRPC);
	l_result &= createRTV(m_DXRPC);

	l_result &= createDSVDescriptorHeap(m_DXRPC);
	l_result &= createDSV(m_DXRPC);

	// Setup root signature.
	CD3DX12_DESCRIPTOR_RANGE1 l_bassPassRT0DescRange;
	l_bassPassRT0DescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE1 l_samplerDescRange;
	l_samplerDescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	CD3DX12_ROOT_PARAMETER1 l_rootParams[2];
	l_rootParams[0].InitAsDescriptorTable(1, &l_bassPassRT0DescRange, D3D12_SHADER_VISIBILITY_PIXEL);
	l_rootParams[1].InitAsDescriptorTable(1, &l_samplerDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

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
	m_DXRPC->m_scissor.right = (uint64_t)m_DXRPC->m_viewport.Width;
	m_DXRPC->m_scissor.bottom = (uint64_t)m_DXRPC->m_viewport.Height;

	// Setup PSO.
	m_DXRPC->m_PSODesc.SampleMask = UINT_MAX;
	m_DXRPC->m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_DXRPC->m_PSODesc.SampleDesc.Count = 1;

	l_result &= createRootSignature(m_DXRPC);
	l_result &= createPSO(m_DXRPC, m_DXSPC);

	l_result &= createSyncPrimitives(m_DXRPC);

	return true;
}

bool DX12FinalBlendPass::initializeShaders()
{
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

bool DX12FinalBlendPass::update()
{
	auto l_frameIndex = m_DXRPC->m_currentFrameIndex;

	auto l_MDC = getDX12MeshDataComponent(MeshShapeType::QUAD);

	recordCommandBegin(m_DXRPC, l_frameIndex);

	m_DXRPC->m_commandLists[l_frameIndex]->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_DXRPC->m_renderTargets[l_frameIndex]->m_texture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	recordActivateRenderPass(m_DXRPC, l_frameIndex);

	ID3D12DescriptorHeap* l_heaps[] = { DX12RenderingBackendComponent::get().m_CSUHeap, DX12RenderingBackendComponent::get().m_samplerHeap };
	recordBindDescHeaps(m_DXRPC, l_frameIndex, 2, l_heaps);

	recordBindRTForRead(m_DXRPC, l_frameIndex, DX12LightPass::getDX12RPC()->m_renderTargets[0]);

	recordBindSRVDescTable(m_DXRPC, l_frameIndex, 0, DX12LightPass::getDX12RPC()->m_SRVs[0]);
	recordBindSamplerDescTable(m_DXRPC, l_frameIndex, 1, m_DXSPC);

	recordDrawCall(m_DXRPC, l_frameIndex, l_MDC);

	recordBindRTForWrite(m_DXRPC, l_frameIndex, DX12LightPass::getDX12RPC()->m_renderTargets[0]);

	m_DXRPC->m_commandLists[l_frameIndex]->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_DXRPC->m_renderTargets[l_frameIndex]->m_texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	recordCommandEnd(m_DXRPC, l_frameIndex);

	return true;
}

bool DX12FinalBlendPass::render()
{
	auto l_frameIndex = m_DXRPC->m_currentFrameIndex;

	executeCommandList(m_DXRPC, l_frameIndex);

	// Present the frame.
	DX12RenderingBackendComponent::get().m_swapChain->Present(1, 0);

	// Schedule a Signal command in the queue.
	const UINT64 l_currentFenceValue = m_DXRPC->m_fenceStatus[l_frameIndex];
	m_DXRPC->m_commandQueue->Signal(m_DXRPC->m_fence, l_currentFenceValue);

	auto l_nextFrameIndex = DX12RenderingBackendComponent::get().m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_DXRPC->m_fence->GetCompletedValue() < l_currentFenceValue)
	{
		waitFrame(m_DXRPC, l_nextFrameIndex);
	}

	// Set the fence value for the next frame.
	m_DXRPC->m_fenceStatus[l_nextFrameIndex] = l_currentFenceValue + 1;

	// Update the frame index.
	m_DXRPC->m_currentFrameIndex = l_nextFrameIndex;

	return true;
}

bool DX12FinalBlendPass::terminate()
{
	destroyDX12RenderPassComponent(m_DXRPC);
	return true;
}

bool DX12FinalBlendPass::resize()
{
	return true;
}

bool DX12FinalBlendPass::reloadShaders()
{
	return true;
}

DX12RenderPassComponent * DX12FinalBlendPass::getDX12RPC()
{
	return m_DXRPC;
}
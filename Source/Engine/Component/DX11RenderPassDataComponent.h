#pragma once
#include "RenderPassDataComponent.h"
#include "../RenderingBackend/DX11RenderingBackend/DX11Headers.h"

class DX11ResourceBinder : public IResourceBinder
{
public:
	ID3D11SamplerState* m_Sampler = 0;
	ID3D11Buffer* m_Buffer = 0;
	ID3D11ShaderResourceView* m_SRV = 0;
	ID3D11UnorderedAccessView* m_UAV = 0;
};

class DX11PipelineStateObject : public IPipelineStateObject
{
public:
	ID3D11InputLayout* m_InputLayout = 0;
	D3D11_DEPTH_STENCIL_DESC m_DepthStencilDesc = {};
	ID3D11DepthStencilState* m_DepthStencilState = 0;
	D3D11_BLEND_DESC m_BlendDesc = {};
	ID3D11BlendState* m_BlendState = 0;
	D3D11_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
	D3D11_RASTERIZER_DESC m_RasterizerDesc = {};
	ID3D11RasterizerState* m_RasterizerState = 0;
	D3D11_VIEWPORT m_Viewport = {};
};

class DX11CommandList : public ICommandList
{
};

class DX11CommandQueue : public ICommandQueue
{
};

class DX11Semaphore : public ISemaphore
{
};

class DX11Fence : public IFence
{
};

class DX11RenderPassDataComponent : public RenderPassDataComponent
{
public:
	D3D11_RENDER_TARGET_VIEW_DESC m_RTVDesc = {};
	std::vector<ID3D11RenderTargetView*> m_RTVs;

	D3D11_DEPTH_STENCIL_VIEW_DESC m_DSVDesc = {};
	ID3D11DepthStencilView* m_DSV = 0;
};
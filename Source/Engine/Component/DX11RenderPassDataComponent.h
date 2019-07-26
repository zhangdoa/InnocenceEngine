#pragma once
#include "RenderPassDataComponent.h"
#include "../RenderingBackend/DX11RenderingBackend/DX11Headers.h"

class DX11PipelineStateObject : public IPipelineStateObject
{
public:
	std::deque<std::function<void()>> m_Activate;
	std::deque<std::function<void()>> m_Deactivate;
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

	D3D11_DEPTH_STENCIL_DESC m_depthStencilDesc;
	ID3D11DepthStencilState* m_depthStencilState;

	D3D11_VIEWPORT m_viewport = {};

	D3D11_RASTERIZER_DESC m_rasterizerDesc = {};
	ID3D11RasterizerState* m_rasterizerState = 0;
};
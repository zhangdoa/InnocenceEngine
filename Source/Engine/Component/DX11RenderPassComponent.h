#pragma once
#include "../Common/InnoType.h"
#include "../System/RenderingBackend/DX11RenderingBackend/DX11Headers.h"
#include "RenderPassComponent.h"
#include "DX11TextureDataComponent.h"

class DX11RenderPassComponent : public RenderPassComponent
{
public:
	DX11RenderPassComponent() {};
	~DX11RenderPassComponent() {};

	std::vector<DX11TextureDataComponent*> m_DXTDCs;
	D3D11_RENDER_TARGET_VIEW_DESC m_RTVDesc = {};
	std::vector<ID3D11RenderTargetView*> m_RTVs;

	DX11TextureDataComponent* m_depthStencilDXTDC;
	D3D11_DEPTH_STENCIL_VIEW_DESC m_DSVDesc = {};
	ID3D11DepthStencilView* m_DSV = 0;

	D3D11_DEPTH_STENCIL_DESC m_depthStencilDesc;
	ID3D11DepthStencilState* m_depthStencilState;

	D3D11_VIEWPORT m_viewport = {};

	D3D11_RASTERIZER_DESC m_rasterizerDesc = {};
	ID3D11RasterizerState* m_rasterizerState = 0;
};
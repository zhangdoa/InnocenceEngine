#pragma once
#include "../common/InnoType.h"
#include "../system/DX11RenderingBackend/DX11Headers.h"
#include "TextureDataComponent.h"
#include "DX11TextureDataComponent.h"

class DX11RenderPassComponent
{
public:
	DX11RenderPassComponent() {};
	~DX11RenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<DX11TextureDataComponent*> m_DXTDCs;

	std::vector<ID3D11RenderTargetView*> m_renderTargetViews;

	D3D11_TEXTURE2D_DESC m_depthBufferDesc = D3D11_TEXTURE2D_DESC();
	ID3D11Texture2D* m_depthStencilBuffer = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc = D3D11_DEPTH_STENCIL_VIEW_DESC();
	ID3D11DepthStencilView* m_depthStencilView = 0;

	D3D11_VIEWPORT m_viewport = D3D11_VIEWPORT();
};
#pragma once
#include "../common/InnoType.h"
#include "../system/DXHeaders.h"
#include "TextureDataComponent.h"
#include "DXTextureDataComponent.h"

class DXRenderPassComponent
{
public:
	DXRenderPassComponent() {};
	~DXRenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<DXTextureDataComponent*> m_DXTDCs;

	std::vector<ID3D11RenderTargetView*> m_renderTargetViews;

	D3D11_TEXTURE2D_DESC m_depthBufferDesc = D3D11_TEXTURE2D_DESC();
	ID3D11Texture2D* m_depthStencilBuffer = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc = D3D11_DEPTH_STENCIL_VIEW_DESC();
	ID3D11DepthStencilView* m_depthStencilView = 0;

	D3D11_VIEWPORT m_viewport = D3D11_VIEWPORT();
};
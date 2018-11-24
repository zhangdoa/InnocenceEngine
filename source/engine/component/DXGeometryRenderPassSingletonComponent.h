#pragma once
#include "../common/InnoType.h"
#include "TextureDataComponent.h"
#include "DXTextureDataComponent.h"

class DXGeometryRenderPassSingletonComponent
{
public:
	~DXGeometryRenderPassSingletonComponent() {};
	
	static DXGeometryRenderPassSingletonComponent& getInstance()
	{
		static DXGeometryRenderPassSingletonComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;

	D3D11_BUFFER_DESC m_constantBufferDesc;
	ID3D11Buffer* m_constantBuffer;

	D3D11_SAMPLER_DESC m_samplerDesc;
	ID3D11SamplerState* m_samplerState;

	D3D11_TEXTURE2D_DESC m_renderTargetTextureDesc;
	std::vector<ID3D11Texture2D*> m_renderTargetTextures;

	D3D11_RENDER_TARGET_VIEW_DESC m_renderTargetViewDesc;
	std::vector<ID3D11RenderTargetView*> m_renderTargetViews;

	D3D11_SHADER_RESOURCE_VIEW_DESC m_shaderResourceViewDesc;
	std::vector<ID3D11ShaderResourceView*> m_shaderResourceViews;

	D3D11_TEXTURE2D_DESC m_depthBufferDesc;
	ID3D11Texture2D* m_depthStencilBuffer;

	D3D11_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc;
	ID3D11DepthStencilView* m_depthStencilView;

	D3D11_VIEWPORT m_viewport;

private:
	DXGeometryRenderPassSingletonComponent() {};
};

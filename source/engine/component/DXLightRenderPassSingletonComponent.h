#pragma once
#include "../common/InnoType.h"
#include "TextureDataComponent.h"
#include "DXTextureDataComponent.h"

class DXLightRenderPassSingletonComponent
{
public:
	~DXLightRenderPassSingletonComponent() {};
	
	static DXLightRenderPassSingletonComponent& getInstance()
	{
		static DXLightRenderPassSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;

	D3D11_SAMPLER_DESC m_samplerDesc;
	ID3D11SamplerState* m_samplerState;

	D3D11_TEXTURE2D_DESC m_renderTargetTextureDesc;
	ID3D11Texture2D* m_renderTargetTexture;

	D3D11_RENDER_TARGET_VIEW_DESC m_renderTargetViewDesc;
	ID3D11RenderTargetView* m_renderTargetView;

	D3D11_SHADER_RESOURCE_VIEW_DESC m_shaderResourceViewDesc;
	ID3D11ShaderResourceView* m_shaderResourceView;

	D3D11_TEXTURE2D_DESC m_depthBufferDesc;
	ID3D11Texture2D* m_depthStencilBuffer;

	D3D11_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc;
	ID3D11DepthStencilView* m_depthStencilView;

	D3D11_VIEWPORT m_viewport;

private:
	DXLightRenderPassSingletonComponent() {};
};

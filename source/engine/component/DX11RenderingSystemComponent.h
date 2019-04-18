#pragma once
#include "../common/InnoType.h"
#include "../component/DX11MeshDataComponent.h"
#include "../component/DX11TextureDataComponent.h"

struct DX11CBuffer
{
	D3D11_BUFFER_DESC m_CBufferDesc = D3D11_BUFFER_DESC();
	ID3D11Buffer* m_CBufferPtr = 0;
};

struct SkyCBufferData
{
	mat4 p_inv;
	mat4 r_inv;
	vec2 viewportSize;
	vec2 padding1;
};

class DX11RenderingSystemComponent
{
public:
	~DX11RenderingSystemComponent() {};

	static DX11RenderingSystemComponent& get()
	{
		static DX11RenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	TVec2<unsigned int> m_refreshRate = TVec2<unsigned int>(0, 1);

	int m_videoCardMemory;
	char m_videoCardDescription[128];

	IDXGIFactory* m_factory;

	DXGI_ADAPTER_DESC m_adapterDesc;
	IDXGIAdapter* m_adapter;
	IDXGIOutput* m_adapterOutput;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;

	DXGI_SWAP_CHAIN_DESC m_swapChainDesc;
	IDXGISwapChain* m_swapChain;

	ID3D11Texture2D* m_renderTargetTexture;
	ID3D11RenderTargetView* m_renderTargetView;

	D3D11_TEXTURE2D_DESC m_depthTextureDesc;
	D3D11_DEPTH_STENCIL_DESC m_depthStencilDesc;
	ID3D11Texture2D* m_depthStencilTexture;
	ID3D11DepthStencilState* m_defaultDepthStencilState;

	D3D11_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc;
	ID3D11DepthStencilView* m_depthStencilView;

	D3D11_RASTERIZER_DESC m_rasterDescForward;
	ID3D11RasterizerState* m_rasterStateForward;

	D3D11_RASTERIZER_DESC m_rasterDescDeferred;
	ID3D11RasterizerState* m_rasterStateDeferred;

	D3D11_VIEWPORT m_viewport;

	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();
	D3D11_RENDER_TARGET_VIEW_DESC deferredPassRTVDesc = D3D11_RENDER_TARGET_VIEW_DESC();

	DX11CBuffer m_cameraCBuffer;
	DX11CBuffer m_materialCBuffer;
	DX11CBuffer m_meshCBuffer;
	DX11CBuffer m_sunCBuffer;
	DX11CBuffer m_pointLightCBuffer;
	DX11CBuffer m_sphereLightCBuffer;
	DX11CBuffer m_skyCBuffer;

	SkyCBufferData m_skyCBufferData;
private:
	DX11RenderingSystemComponent() {};
};

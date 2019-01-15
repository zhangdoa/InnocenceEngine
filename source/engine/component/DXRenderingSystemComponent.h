#pragma once
#include "../common/InnoType.h"
#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"

class DXRenderingSystemComponent
{
public:
	~DXRenderingSystemComponent() {};
	
	static DXRenderingSystemComponent& get()
	{
		static DXRenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	bool m_vsync_enabled = true;
	TVec2<unsigned int> m_refreshRate = TVec2<unsigned int>(0, 1);

	int m_videoCardMemory;
	char m_videoCardDescription[128];

	DXGI_SWAP_CHAIN_DESC m_swapChainDesc;
	IDXGISwapChain* m_swapChain;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;

	ID3D11Texture2D* m_renderTargetTexture;
	ID3D11RenderTargetView* m_renderTargetView;

	D3D11_TEXTURE2D_DESC m_depthTextureDesc;
	D3D11_DEPTH_STENCIL_DESC m_depthStencilDesc;
	ID3D11Texture2D* m_depthStencilTexture;
	ID3D11DepthStencilState* m_depthStencilState;

	D3D11_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc;
	ID3D11DepthStencilView* m_depthStencilView;

	D3D11_RASTERIZER_DESC m_rasterDescForward;
	ID3D11RasterizerState* m_rasterStateForward;

	D3D11_RASTERIZER_DESC m_rasterDescDeferred;
	ID3D11RasterizerState* m_rasterStateDeferred;

	D3D11_VIEWPORT m_viewport;

	std::unordered_map<EntityID, DXMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, DXTextureDataComponent*> m_textureMap;
	
	DXMeshDataComponent* m_UnitLineDXMDC;
	DXMeshDataComponent* m_UnitQuadDXMDC;
	DXMeshDataComponent* m_UnitCubeDXMDC;
	DXMeshDataComponent* m_UnitSphereDXMDC;

	DXTextureDataComponent* m_iconTemplate_OBJ;
	DXTextureDataComponent* m_iconTemplate_PNG;
	DXTextureDataComponent* m_iconTemplate_SHADER;
	DXTextureDataComponent* m_iconTemplate_UNKNOWN;

	DXTextureDataComponent* m_iconTemplate_DirectionalLight;
	DXTextureDataComponent* m_iconTemplate_PointLight;
	DXTextureDataComponent* m_iconTemplate_SphereLight;

	DXTextureDataComponent* m_basicNormalDXTDC;
	DXTextureDataComponent* m_basicAlbedoDXTDC;
	DXTextureDataComponent* m_basicMetallicDXTDC;
	DXTextureDataComponent* m_basicRoughnessDXTDC;
	DXTextureDataComponent* m_basicAODXTDC;

private:
	DXRenderingSystemComponent() {};
};

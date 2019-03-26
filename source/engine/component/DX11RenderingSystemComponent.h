#pragma once
#include "../common/InnoType.h"
#include "../component/DX11MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/DX11TextureDataComponent.h"

struct GPassCameraCBufferData
{
	mat4 m_CamProjOriginal;
	mat4 m_CamProjJittered;
	mat4 m_CamRot;
	mat4 m_CamTrans;
	mat4 m_CamRot_prev;
	mat4 m_CamTrans_prev;
};

struct GPassMeshCBufferData
{
	mat4 m;
	mat4 m_prev;
	mat4 m_normalMat;
};

struct GPassTextureCBufferData
{
	vec4 albedo;
	vec4 MRA;
	int useNormalTexture = true;
	int useAlbedoTexture = true;
	int useMetallicTexture = true;
	int useRoughnessTexture = true;
	int useAOTexture = true;
	int padding1 = true;
	int padding2 = true;
	int padding3 = true;
};

struct GPassMeshDataPack
{
	size_t indiceSize;
	GPassMeshCBufferData meshCBuffer;
	GPassTextureCBufferData textureCBuffer;
	DX11MeshDataComponent* DXMDC;
	MeshPrimitiveTopology meshPrimitiveTopology;
	DX11TextureDataComponent* normalDXTDC;
	DX11TextureDataComponent* albedoDXTDC;
	DX11TextureDataComponent* metallicDXTDC;
	DX11TextureDataComponent* roughnessDXTDC;
	DX11TextureDataComponent* AODXTDC;
};

struct LPassCBufferData
{
	vec4 viewPos;
	vec4 lightDir;
	vec4 color;
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

	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();
	D3D11_RENDER_TARGET_VIEW_DESC deferredPassRTVDesc = D3D11_RENDER_TARGET_VIEW_DESC();

	GPassCameraCBufferData m_GPassCameraCBufferData;
	std::queue<GPassMeshDataPack> m_GPassMeshDataQueue;
	LPassCBufferData m_LPassCBufferData;

	DX11MeshDataComponent* m_UnitLineDXMDC;
	DX11MeshDataComponent* m_UnitQuadDXMDC;
	DX11MeshDataComponent* m_UnitCubeDXMDC;
	DX11MeshDataComponent* m_UnitSphereDXMDC;

	DX11TextureDataComponent* m_iconTemplate_OBJ;
	DX11TextureDataComponent* m_iconTemplate_PNG;
	DX11TextureDataComponent* m_iconTemplate_SHADER;
	DX11TextureDataComponent* m_iconTemplate_UNKNOWN;

	DX11TextureDataComponent* m_iconTemplate_DirectionalLight;
	DX11TextureDataComponent* m_iconTemplate_PointLight;
	DX11TextureDataComponent* m_iconTemplate_SphereLight;

	DX11TextureDataComponent* m_basicNormalDXTDC;
	DX11TextureDataComponent* m_basicAlbedoDXTDC;
	DX11TextureDataComponent* m_basicMetallicDXTDC;
	DX11TextureDataComponent* m_basicRoughnessDXTDC;
	DX11TextureDataComponent* m_basicAODXTDC;

private:
	DX11RenderingSystemComponent() {};
};

#pragma once
#include "../common/InnoType.h"
#include "../component/DXMeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/DXTextureDataComponent.h"

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

struct GPassRenderingDataPack
{
	size_t indiceSize;
	GPassMeshCBufferData meshCBuffer;
	GPassTextureCBufferData textureCBuffer;
	DXMeshDataComponent* DXMDC;
	MeshPrimitiveTopology meshPrimitiveTopology;
	DXTextureDataComponent* normalDXTDC;
	DXTextureDataComponent* albedoDXTDC;
	DXTextureDataComponent* metallicDXTDC;
	DXTextureDataComponent* roughnessDXTDC;
	DXTextureDataComponent* AODXTDC;
};

struct LPassCBufferData
{
	vec4 viewPos;
	vec4 lightDir;
	vec4 color;
};

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

	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();
	D3D11_RENDER_TARGET_VIEW_DESC deferredPassRTVDesc = D3D11_RENDER_TARGET_VIEW_DESC();

	std::unordered_map<EntityID, DXMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, DXTextureDataComponent*> m_textureMap;
	
	GPassCameraCBufferData m_GPassCameraCBufferData;
	std::queue<GPassRenderingDataPack> m_GPassRenderingDataQueue;
	LPassCBufferData m_LPassCBufferData;

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

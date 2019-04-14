#pragma once
#include "../common/InnoType.h"
#include "../component/DX12MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/DX12TextureDataComponent.h"

struct DX12CBuffer
{
	D3D12_RESOURCE_DESC m_CBufferDesc = D3D12_RESOURCE_DESC();
	ID3D12Resource* m_CBufferPtr = 0;
};

struct DX12CameraCBufferData
{
	mat4 p_original;
	mat4 p_jittered;
	mat4 r;
	mat4 t;
	mat4 r_prev;
	mat4 t_prev;
	vec4 globalPos;
};

struct DX12MeshCBufferData
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
};

struct DX12TextureCBufferData
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

struct DX12MeshDataPack
{
	size_t indiceSize;
	DX12MeshCBufferData meshCBuffer;
	DX12MeshDataComponent* DXMDC;
	DX12TextureCBufferData textureCBuffer;
	MeshPrimitiveTopology meshPrimitiveTopology;
	MeshShapeType meshShapeType;
	DX12TextureDataComponent* normalDXTDC;
	DX12TextureDataComponent* albedoDXTDC;
	DX12TextureDataComponent* metallicDXTDC;
	DX12TextureDataComponent* roughnessDXTDC;
	DX12TextureDataComponent* AODXTDC;
	VisiblilityType visiblilityType;
};

struct DirectionalLightCBufferData
{
	vec4 dir;
	vec4 luminance;
};

// w component of luminance is attenuationRadius
struct PointLightCBufferData
{
	vec4 pos;
	vec4 luminance;
	//float attenuationRadius;
};

// w component of luminance is sphereRadius
struct SphereLightCBufferData
{
	vec4 pos;
	vec4 luminance;
	//float sphereRadius;
};

class DX12RenderingSystemComponent
{
public:
	~DX12RenderingSystemComponent() {};

	static DX12RenderingSystemComponent& get()
	{
		static DX12RenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	TVec2<unsigned int> m_refreshRate = TVec2<unsigned int>(0, 1);

	int m_videoCardMemory;
	char m_videoCardDescription[128];

	ID3D12Debug1* m_debugInterface;

	IDXGIFactory4* m_factory;

	DXGI_ADAPTER_DESC m_adapterDesc;
	IDXGIAdapter4* m_adapter;
	IDXGIOutput* m_adapterOutput;

	ID3D12Device2* m_device;

	D3D12_COMMAND_QUEUE_DESC m_commandQueueDesc;
	ID3D12CommandQueue* m_commandQueue;

	DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc;
	IDXGISwapChain4* m_swapChain;

	ID3D12DescriptorHeap* m_renderTargetViewHeap;
	ID3D12Resource* m_backBufferRenderTarget[2];
	D3D12_DESCRIPTOR_HEAP_DESC m_renderTargetViewHeapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetViewHandle;
	unsigned int m_bufferIndex;

	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12GraphicsCommandList* m_commandList;

	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	unsigned long long m_fenceValue;

	D3D12_DEPTH_STENCIL_DESC m_depthStencilDesc;

	D3D12_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc;

	D3D12_RASTERIZER_DESC m_rasterDescForward;

	D3D12_RASTERIZER_DESC m_rasterDescDeferred;

	D3D12_VIEWPORT m_viewport;

	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();
	D3D12_RENDER_TARGET_VIEW_DESC deferredPassRTVDesc = D3D12_RENDER_TARGET_VIEW_DESC();

	DX12CameraCBufferData m_cameraCBufferData;
	std::queue<DX12MeshDataPack> m_meshDataQueue;
	DirectionalLightCBufferData m_directionalLightCBufferData;
	const unsigned int m_maxPointLights = 64;
	std::vector<PointLightCBufferData> m_PointLightCBufferDatas;

	const unsigned int m_maxSphereLights = 64;
	std::vector<SphereLightCBufferData> m_SphereLightCBufferDatas;

	DX12MeshDataComponent* m_UnitLineDXMDC;
	DX12MeshDataComponent* m_UnitQuadDXMDC;
	DX12MeshDataComponent* m_UnitCubeDXMDC;
	DX12MeshDataComponent* m_UnitSphereDXMDC;

	DX12TextureDataComponent* m_iconTemplate_OBJ;
	DX12TextureDataComponent* m_iconTemplate_PNG;
	DX12TextureDataComponent* m_iconTemplate_SHADER;
	DX12TextureDataComponent* m_iconTemplate_UNKNOWN;

	DX12TextureDataComponent* m_iconTemplate_DirectionalLight;
	DX12TextureDataComponent* m_iconTemplate_PointLight;
	DX12TextureDataComponent* m_iconTemplate_SphereLight;

	DX12TextureDataComponent* m_basicNormalDXTDC;
	DX12TextureDataComponent* m_basicAlbedoDXTDC;
	DX12TextureDataComponent* m_basicMetallicDXTDC;
	DX12TextureDataComponent* m_basicRoughnessDXTDC;
	DX12TextureDataComponent* m_basicAODXTDC;

private:
	DX12RenderingSystemComponent() {};
};

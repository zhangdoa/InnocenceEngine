#pragma once
#include "../common/InnoType.h"
#include "../component/DX12MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/DX12TextureDataComponent.h"

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
	DX12MeshDataComponent* DXMDC;
	MeshPrimitiveTopology meshPrimitiveTopology;
	DX12TextureDataComponent* normalDXTDC;
	DX12TextureDataComponent* albedoDXTDC;
	DX12TextureDataComponent* metallicDXTDC;
	DX12TextureDataComponent* roughnessDXTDC;
	DX12TextureDataComponent* AODXTDC;
};

struct LPassCBufferData
{
	vec4 viewPos;
	vec4 lightDir;
	vec4 color;
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

	IDXGIFactory4* m_factory;

	DXGI_ADAPTER_DESC m_adapterDesc;
	IDXGIAdapter* m_adapter;
	IDXGIOutput* m_adapterOutput;

	ID3D12Device* m_device;

	D3D12_COMMAND_QUEUE_DESC m_commandQueueDesc;
	ID3D12CommandQueue* m_commandQueue;

	DXGI_SWAP_CHAIN_DESC m_swapChainDesc;
	IDXGISwapChain3* m_swapChain;

	ID3D12DescriptorHeap* m_renderTargetViewHeap;
	ID3D12Resource* m_backBufferRenderTarget[2];
	D3D12_DESCRIPTOR_HEAP_DESC m_renderTargetViewHeapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetViewHandle;
	unsigned int m_bufferIndex;

	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12GraphicsCommandList* m_commandList;

	ID3D12PipelineState* m_pipelineState;

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

	GPassCameraCBufferData m_GPassCameraCBufferData;
	std::queue<GPassMeshDataPack> m_GPassMeshDataQueue;
	LPassCBufferData m_LPassCBufferData;

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

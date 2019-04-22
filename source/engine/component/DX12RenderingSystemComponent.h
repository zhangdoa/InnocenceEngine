#pragma once
#include "../common/InnoType.h"
#include "../component/DX12RenderPassComponent.h"
#include "../component/DX12ShaderProgramComponent.h"
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

	ID3D12CommandAllocator* m_commandAllocator;

	D3D12_COMMAND_QUEUE_DESC m_commandQueueDesc;
	ID3D12CommandQueue* m_commandQueue;

	DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc;
	IDXGISwapChain4* m_swapChain;

	DX12ShaderProgramComponent* m_swapChainDXSPC;
	DX12RenderPassComponent* m_swapChainDXRPC;

	D3D12_DEPTH_STENCIL_DESC m_depthStencilDesc;

	D3D12_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc;

	D3D12_RASTERIZER_DESC m_rasterDescForward;

	D3D12_RASTERIZER_DESC m_rasterDescDeferred;

	D3D12_VIEWPORT m_viewport;

	RenderPassDesc m_deferredRenderPassDesc = RenderPassDesc();

private:
	DX12RenderingSystemComponent() {};
};

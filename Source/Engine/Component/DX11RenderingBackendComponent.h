#pragma once
#include "../Common/InnoType.h"
#include "../component/DX11RenderPassDataComponent.h"
#include "../component/DX11MeshDataComponent.h"
#include "../component/DX11TextureDataComponent.h"

class DX11RenderingBackendComponent
{
public:
	~DX11RenderingBackendComponent() {};

	static DX11RenderingBackendComponent& get()
	{
		static DX11RenderingBackendComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	TVec2<unsigned int> m_refreshRate = TVec2<unsigned int>(0, 1);

	int m_videoCardMemory;
	char m_videoCardDescription[128];

	IDXGIFactory* m_factory;

	DXGI_ADAPTER_DESC m_adapterDesc;
	IDXGIAdapter* m_adapter;
	IDXGIOutput* m_adapterOutput;

	ID3D11Device5* m_device;
	ID3D11DeviceContext4* m_deviceContext;

	DXGI_SWAP_CHAIN_DESC m_swapChainDesc;
	IDXGISwapChain4* m_swapChain;
	std::vector<ID3D11Texture2D*> m_swapChainTextures;

	RenderPassDesc m_deferredRenderPassDesc = RenderPassDesc();

	DX11ConstantBuffer m_cameraConstantBuffer;
	DX11ConstantBuffer m_materialConstantBuffer;
	DX11ConstantBuffer m_meshConstantBuffer;
	DX11ConstantBuffer m_sunConstantBuffer;
	DX11ConstantBuffer m_pointLightConstantBuffer;
	DX11ConstantBuffer m_sphereLightConstantBuffer;

	DX11ConstantBuffer m_skyConstantBuffer;

	DX11ConstantBuffer m_dispatchParamsConstantBuffer;

	DX11StructuredBuffer m_gridFrustumsStructuredBuffer;
	DX11StructuredBuffer m_lightIndexListStructuredBuffer;
	DX11StructuredBuffer m_lightListIndexCounterStructuredBuffer;

private:
	DX11RenderingBackendComponent() {};
};

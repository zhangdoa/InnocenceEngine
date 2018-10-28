#pragma once
#include "../common/InnoType.h"
#include "../../component/DXMeshDataComponent.h"
#include "../../component/DXTextureDataComponent.h"

class DXRenderingSystemSingletonComponent
{
public:
	~DXRenderingSystemSingletonComponent() {};
	
	static DXRenderingSystemSingletonComponent& getInstance()
	{
		static DXRenderingSystemSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	bool m_vsync_enabled;
	int m_videoCardMemory;
	char m_videoCardDescription[128];
	IDXGISwapChain* m_swapChain;
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;
	ID3D11RenderTargetView* m_renderTargetView;
	ID3D11Texture2D* m_depthStencilBuffer;
	ID3D11DepthStencilState* m_depthStencilState;
	ID3D11DepthStencilView* m_depthStencilView;
	ID3D11RasterizerState* m_rasterState;

	std::unordered_map<EntityID, DXMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, DXTextureDataComponent*> m_textureMap;

private:
	DXRenderingSystemSingletonComponent() {};
};

#pragma once
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "component/EnvironmentRenderPassSingletonComponent.h"
#include "component/ShadowRenderPassSingletonComponent.h"
#include "component/GeometryRenderPassSingletonComponent.h"
#include "component/LightRenderPassSingletonComponent.h"
#include "component/FinalRenderPassSingletonComponent.h"
#include "component/RenderingSystemSingletonComponent.h"
#include "component/AssetSystemSingletonComponent.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IGameSystem.h"
#include "interface/IAssetSystem.h"

#include <d3d11.h>
#include <DirectXMath.h>

#include "component/WindowSystemSingletonComponent.h"

extern IMemorySystem* g_pMemorySystem;
extern IAssetSystem* g_pAssetSystem;
extern IGameSystem* g_pGameSystem;

class DXRenderingSystem : public IRenderingSystem
{
public:
	DXRenderingSystem() {};
	~DXRenderingSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

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
	DirectX::XMMATRIX m_projectionMatrix;
	DirectX::XMMATRIX m_worldMatrix;
	DirectX::XMMATRIX m_orthoMatrix;

	void beginScene(float r, float g, float b, float a);
	void endScene();
};

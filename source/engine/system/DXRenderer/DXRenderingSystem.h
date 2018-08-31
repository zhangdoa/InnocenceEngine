#pragma once

#include "../../component/DXFinalRenderPassSingletonComponent.h"

#include "../../interface/IRenderingSystem.h"
#include "../../interface/IMemorySystem.h"
#include "../../interface/IGameSystem.h"
#include "../../interface/IAssetSystem.h"

#include <sstream>

#include "../../component/RenderingSystemSingletonComponent.h"
#include "../../component/WindowSystemSingletonComponent.h"
#include "../../component/AssetSystemSingletonComponent.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#include "DXHeaders.h"

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

	void initializeFinalBlendPass();

	void initializeShader(shaderType shaderType, const std::wstring & shaderFilePath);
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);

	void initializeDefaultGraphicPrimtives();
	void initializeGraphicPrimtivesOfComponents();
	void initializeMesh(MeshDataComponent* DXMeshDataComponent);

	void updateFinalBlendPass();

	void updateShaderParameter(shaderType shaderType, ID3D11Buffer* matrixBuffer, DirectX::XMMATRIX parameterValue);
	void beginScene(float r, float g, float b, float a);
	void endScene();
};

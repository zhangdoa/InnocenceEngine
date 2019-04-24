#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/MaterialDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DX11MeshDataComponent.h"
#include "../../component/DX11TextureDataComponent.h"
#include "../../component/DX11RenderPassComponent.h"
#include "../../component/DX11ShaderProgramComponent.h"
#include "../../component/DX11RenderingSystemComponent.h"

INNO_PRIVATE_SCOPE DX11RenderingSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool render();
	bool terminate();
	bool initializeComponentPool();
	bool resize();

	void loadDefaultAssets();
	bool generateGPUBuffers();

	DX11MeshDataComponent* addDX11MeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	DX11TextureDataComponent* addDX11TextureDataComponent();

	DX11MeshDataComponent* getDX11MeshDataComponent(EntityID meshID);
	DX11TextureDataComponent* getDX11TextureDataComponent(EntityID textureID);

	DX11MeshDataComponent* getDX11MeshDataComponent(MeshShapeType MeshShapeType);
	DX11TextureDataComponent* getDX11TextureDataComponent(TextureUsageType TextureUsageType);
	DX11TextureDataComponent* getDX11TextureDataComponent(FileExplorerIconType iconType);
	DX11TextureDataComponent* getDX11TextureDataComponent(WorldEditorIconType iconType);

	DX11RenderPassComponent* addDX11RenderPassComponent(unsigned int RTNum, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc, TextureDataDesc RTDesc);

	bool initializeDX11MeshDataComponent(DX11MeshDataComponent* rhs);
	bool initializeDX11TextureDataComponent(DX11TextureDataComponent* rhs);

	void drawMesh(DX11MeshDataComponent * DXMDC);
	void activateTexture(ShaderType shaderType, unsigned int startSlot, DX11TextureDataComponent* DXTDC);

	DX11ShaderProgramComponent* addDX11ShaderProgramComponent(EntityID rhs);

	bool createConstantBuffer(DX11ConstantBuffer& arg);
	bool createStructuredBuffer(void* initialData, DX11StructuredBuffer& arg);

	bool destroyStructuredBuffer(DX11StructuredBuffer& arg);

	bool initializeDX11ShaderProgramComponent(DX11ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool activateDX11ShaderProgramComponent(DX11ShaderProgramComponent* rhs);

	void updateConstantBuffer(const DX11ConstantBuffer& ConstantBuffer, void* ConstantBufferValue);
	void bindConstantBuffer(ShaderType shaderType, unsigned int startSlot, const DX11ConstantBuffer& ConstantBuffer);

	void bindStructuredBufferForWrite(ShaderType shaderType, unsigned int startSlot, const DX11StructuredBuffer& StructuredBuffer);
	void bindStructuredBufferForRead(ShaderType shaderType, unsigned int startSlot, const DX11StructuredBuffer& StructuredBuffer);

	void cleanRTV(vec4 color, ID3D11RenderTargetView* RTV);
	void cleanDSV(ID3D11DepthStencilView* DSV);
}
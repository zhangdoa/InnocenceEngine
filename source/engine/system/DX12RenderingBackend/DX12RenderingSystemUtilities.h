#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/MaterialDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DX12MeshDataComponent.h"
#include "../../component/DX12TextureDataComponent.h"
#include "../../component/DX12ShaderProgramComponent.h"
#include "../../component/DX12RenderPassComponent.h"

INNO_PRIVATE_SCOPE DX12RenderingSystemNS
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

	DX12MeshDataComponent* addDX12MeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	DX12TextureDataComponent* addDX12TextureDataComponent();

	DX12MeshDataComponent* getDX12MeshDataComponent(EntityID meshID);
	DX12TextureDataComponent* getDX12TextureDataComponent(EntityID textureID);

	DX12MeshDataComponent* getDX12MeshDataComponent(MeshShapeType MeshShapeType);
	DX12TextureDataComponent* getDX12TextureDataComponent(TextureUsageType TextureUsageType);
	DX12TextureDataComponent* getDX12TextureDataComponent(FileExplorerIconType iconType);
	DX12TextureDataComponent* getDX12TextureDataComponent(WorldEditorIconType iconType);

	DX12RenderPassComponent* addDX12RenderPassComponent(EntityID rhs);
	bool createRootSignature(DX12RenderPassComponent* DXRPC);
	bool createPSO(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC);
	bool initializeDX12RenderPassComponent(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC);

	bool initializeDX12MeshDataComponent(DX12MeshDataComponent* rhs);
	bool initializeDX12TextureDataComponent(DX12TextureDataComponent* rhs);

	void recordDrawCall(EntityID rhs);
	void recordDrawCall(MeshDataComponent* MDC);
	void recordDrawCall(size_t indicesSize, DX12MeshDataComponent * DXMDC);

	DX12ShaderProgramComponent* addDX12ShaderProgramComponent(EntityID rhs);

	bool initializeDX12ShaderProgramComponent(DX12ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool activateDX12ShaderProgramComponent(DX12ShaderProgramComponent* rhs);

	void updateShaderParameter(ShaderType shaderType, unsigned int startSlot, ID3D12Resource* CBuffer, size_t size, void* parameterValue);

	void cleanRTV(vec4 color, ID3D12Resource* RTV);
	void cleanDSV(ID3D12Resource* DSV);
}
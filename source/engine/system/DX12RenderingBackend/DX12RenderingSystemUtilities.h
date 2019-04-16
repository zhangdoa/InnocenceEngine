#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DX12MeshDataComponent.h"
#include "../../component/DX12TextureDataComponent.h"
#include "../../component/DX12ShaderProgramComponent.h"
#include "../../component/DX12RenderPassComponent.h"

INNO_PRIVATE_SCOPE DX12RenderingSystemNS
{
	bool initializeComponentPool();

	DX12RenderPassComponent* addDX12RenderPassComponent(EntityID rhs);
	bool createRootSignature(DX12RenderPassComponent* DXRPC);
	bool createPSO(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC);
	bool initializeDX12RenderPassComponent(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC);

	DX12MeshDataComponent* generateDX12MeshDataComponent(MeshDataComponent* rhs);
	DX12TextureDataComponent* generateDX12TextureDataComponent(TextureDataComponent* rhs);

	DX12MeshDataComponent* addDX12MeshDataComponent(EntityID rhs);
	DX12TextureDataComponent* addDX12TextureDataComponent(EntityID rhs);

	DX12MeshDataComponent* getDX12MeshDataComponent(EntityID rhs);
	DX12TextureDataComponent* getDX12TextureDataComponent(EntityID rhs);

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
#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DXMeshDataComponent.h"
#include "../../component/DXTextureDataComponent.h"
#include "../../component/DXShaderProgramComponent.h"
#include "../../component/DXRenderPassComponent.h"

INNO_PRIVATE_SCOPE DXRenderingSystemNS
{
	bool initializeComponentPool();

	DXRenderPassComponent* addDXRenderPassComponent(unsigned int RTNum, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc, TextureDataDesc RTDesc);

	DXMeshDataComponent* generateDXMeshDataComponent(MeshDataComponent* rhs);
	DXTextureDataComponent* generateDXTextureDataComponent(TextureDataComponent* rhs);

	DXMeshDataComponent* addDXMeshDataComponent(EntityID rhs);
	DXTextureDataComponent* addDXTextureDataComponent(EntityID rhs);

	DXMeshDataComponent* getDXMeshDataComponent(EntityID rhs);
	DXTextureDataComponent* getDXTextureDataComponent(EntityID rhs);

	void drawMesh(EntityID rhs);
	void drawMesh(MeshDataComponent* MDC);
	void drawMesh(size_t indicesSize, DXMeshDataComponent * DXMDC);

	DXShaderProgramComponent* addDXShaderProgramComponent(EntityID rhs);

	bool initializeDXShaderProgramComponent(DXShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool activateDXShaderProgramComponent(DXShaderProgramComponent* rhs);

	void updateShaderParameter(ShaderType shaderType, unsigned int startSlot, ID3D11Buffer* CBuffer, size_t size, void* parameterValue);

	void cleanRTV(vec4 color, ID3D11RenderTargetView* RTV);
	void cleanDSV(ID3D11DepthStencilView* DSV);
}
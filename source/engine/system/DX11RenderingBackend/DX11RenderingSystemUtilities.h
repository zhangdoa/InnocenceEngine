#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DX11MeshDataComponent.h"
#include "../../component/DX11TextureDataComponent.h"
#include "../../component/DX11ShaderProgramComponent.h"
#include "../../component/DX11RenderPassComponent.h"

INNO_PRIVATE_SCOPE DX11RenderingSystemNS
{
	bool initializeComponentPool();

	DX11RenderPassComponent* addDX11RenderPassComponent(unsigned int RTNum, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc, TextureDataDesc RTDesc);

	DX11MeshDataComponent* generateDX11MeshDataComponent(MeshDataComponent* rhs);
	DX11TextureDataComponent* generateDX11TextureDataComponent(TextureDataComponent* rhs);

	DX11MeshDataComponent* addDX11MeshDataComponent(EntityID rhs);
	DX11TextureDataComponent* addDX11TextureDataComponent(EntityID rhs);

	DX11MeshDataComponent* getDX11MeshDataComponent(EntityID rhs);
	DX11TextureDataComponent* getDX11TextureDataComponent(EntityID rhs);

	void drawMesh(EntityID rhs);
	void drawMesh(MeshDataComponent* MDC);
	void drawMesh(size_t indicesSize, DX11MeshDataComponent * DXMDC);

	DX11ShaderProgramComponent* addDX11ShaderProgramComponent(EntityID rhs);

	bool initializeDX11ShaderProgramComponent(DX11ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool activateDX11ShaderProgramComponent(DX11ShaderProgramComponent* rhs);

	void updateShaderParameter(ShaderType shaderType, unsigned int startSlot, const std::vector<DX11CBuffer>& DX11CBuffers, void* parameterValue);

	void cleanRTV(vec4 color, ID3D11RenderTargetView* RTV);
	void cleanDSV(ID3D11DepthStencilView* DSV);
}
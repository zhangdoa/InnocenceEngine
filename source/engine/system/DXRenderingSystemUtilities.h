#pragma once
#include "../common/InnoType.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"
#include "../component/DXRenderPassComponent.h"

INNO_PRIVATE_SCOPE DXRenderingSystemNS
{
	ID3D10Blob* loadShaderBuffer(ShaderType shaderType, const std::wstring & shaderFilePath);
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);

	DXRenderPassComponent* addDXRenderPassComponent(unsigned int RTNum, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc, TextureDataDesc RTDesc);

	DXMeshDataComponent* generateDXMeshDataComponent(MeshDataComponent* rhs);
	DXTextureDataComponent* generateDXTextureDataComponent(TextureDataComponent* rhs);
	bool initializeDXTextureDataComponent(DXTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData);

	DXMeshDataComponent* addDXMeshDataComponent(EntityID rhs);
	DXTextureDataComponent* addDXTextureDataComponent(EntityID rhs);

	DXMeshDataComponent* getDXMeshDataComponent(EntityID rhs);
	DXTextureDataComponent* getDXTextureDataComponent(EntityID rhs);

	void drawMesh(EntityID rhs);
	void drawMesh(MeshDataComponent* MDC);
	void drawMesh(size_t indicesSize, DXMeshDataComponent * DXMDC);

	template <class T>
	void updateShaderParameter(ShaderType shaderType, ID3D11Buffer* matrixBuffer, T* parameterValue)
	{
		updateShaderParameterImpl(shaderType, matrixBuffer, sizeof(T), parameterValue);
	}

	void updateShaderParameterImpl(ShaderType shaderType, ID3D11Buffer* matrixBuffer, size_t size, void* parameterValue);

	void cleanRTV(vec4 color, ID3D11RenderTargetView* RTV);
	void cleanDSV(ID3D11DepthStencilView* DSV);
}
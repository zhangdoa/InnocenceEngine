#pragma once
#include "../../Component/DX11TextureDataComponent.h"
#include "../../Component/DX11RenderPassDataComponent.h"
#include "../../Component/DX11ShaderProgramComponent.h"

namespace DX11Helper
{
	D3D11_TEXTURE_DESC GetDX11TextureDataDesc(TextureDataDesc textureDataDesc);
	DXGI_FORMAT GetTextureFormat(TextureDataDesc textureDataDesc);
	unsigned int GetTextureMipLevels(TextureDataDesc textureDataDesc);
	unsigned int GetTextureBindFlags(TextureDataDesc textureDataDesc);
	D3D11_TEXTURE1D_DESC Get1DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc);
	D3D11_TEXTURE2D_DESC Get2DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc);
	D3D11_TEXTURE3D_DESC Get3DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc);
	D3D11_SHADER_RESOURCE_VIEW_DESC GetSRVDesc(TextureDataDesc textureDataDesc, D3D11_TEXTURE_DESC D3D11TextureDesc);
	D3D11_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(TextureDataDesc textureDataDesc, D3D11_TEXTURE_DESC D3D11TextureDesc);
	D3D11_RENDER_TARGET_VIEW_DESC GetRTVDesc(TextureDataDesc textureDataDesc);
	D3D11_DEPTH_STENCIL_VIEW_DESC GetDSVDesc(TextureDataDesc textureDataDesc, DepthStencilDesc DSDesc);

	bool GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX11PipelineStateObject* PSO);
	bool GenerateBlendStateDesc(BlendDesc blendDesc, DX11PipelineStateObject* PSO);
	bool GenerateRasterizerStateDesc(RasterizerDesc rasterizerDesc, DX11PipelineStateObject* PSO);
	bool GenerateViewportStateDesc(ViewportDesc viewportDesc, DX11PipelineStateObject* PSO);

	bool LoadShaderFile(ID3D10Blob** rhs, ShaderType shaderType, const ShaderFilePath& shaderFilePath);
}
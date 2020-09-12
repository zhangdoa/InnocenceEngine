#pragma once
#include "../../Component/DX11TextureDataComponent.h"
#include "../../Component/DX11RenderPassDataComponent.h"
#include "../../Component/DX11ShaderProgramComponent.h"
#include "../IRenderingServer.h"

namespace DX11Helper
{
	template <typename U, typename T>
	bool SetObjectName(U* owner, T* rhs, const char* objectType)
	{
		auto l_Name = std::string(owner->m_InstanceName.c_str());
		l_Name += "_";
		l_Name += objectType;
		auto l_HResult = rhs->SetPrivateData(WKPDID_D3DDebugObjectName, (uint32_t)l_Name.size(), l_Name.c_str());
		if (FAILED(l_HResult))
		{
			InnoLogger::Log(LogLevel::Warning, "DX11RenderingServer: Can't name ", objectType, " with ", l_Name.c_str());
			return false;
		}
		return true;
	}

	D3D11_TEXTURE_DESC GetDX11TextureDesc(TextureDesc textureDesc);
	DXGI_FORMAT GetTextureFormat(TextureDesc textureDesc);
	D3D11_FILTER GetFilterMode(TextureFilterMethod minFilterMethod, TextureFilterMethod magFilterMethod);
	D3D11_TEXTURE_ADDRESS_MODE GetWrapMode(TextureWrapMethod textureWrapMethod);
	uint32_t GetTextureMipLevels(TextureDesc textureDesc);
	uint32_t GetTextureBindFlags(TextureDesc textureDesc);
	uint32_t GetTexturePixelDataSize(TextureDesc textureDesc);
	D3D11_TEXTURE1D_DESC Get1DTextureDesc(D3D11_TEXTURE_DESC textureDesc);
	D3D11_TEXTURE2D_DESC Get2DTextureDesc(D3D11_TEXTURE_DESC textureDesc);
	D3D11_TEXTURE3D_DESC Get3DTextureDesc(D3D11_TEXTURE_DESC textureDesc);
	D3D11_SHADER_RESOURCE_VIEW_DESC GetSRVDesc(TextureDesc textureDesc, D3D11_TEXTURE_DESC D3D11TextureDesc);
	uint32_t GetMipLevels(TextureDesc textureDesc);
	D3D11_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(TextureDesc textureDesc, D3D11_TEXTURE_DESC D3D11TextureDesc);
	D3D11_RENDER_TARGET_VIEW_DESC GetRTVDesc(TextureDesc textureDesc);
	D3D11_DEPTH_STENCIL_VIEW_DESC GetDSVDesc(TextureDesc textureDesc, bool stencilEnable);

	bool ReserveRenderTargets(DX11RenderPassDataComponent* DX11RPDC, IRenderingServer* renderingServer);
	bool CreateRenderTargets(DX11RenderPassDataComponent* DX11RPDC, IRenderingServer* renderingServer);
	bool CreateResourcesBinder(DX11RenderPassDataComponent* DX11RPDC);
	bool CreateViews(DX11RenderPassDataComponent* DX11RPDC, ID3D11Device* device);
	bool CreateStateObjects(DX11RenderPassDataComponent* DX11RPDC, ID3D10Blob* dummyILShaderBuffer, ID3D11Device* device);

	D3D11_COMPARISON_FUNC GetComparisionFunction(ComparisionFunction comparisionFunction);
	D3D11_STENCIL_OP GetStencilOperation(StencilOperation stencilOperation);
	D3D11_BLEND GetBlendFactor(BlendFactor blendFactor);
	D3D11_BLEND_OP GetBlendOperation(BlendOperation blendOperation);
	D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(PrimitiveTopology primitiveTopology);
	D3D11_FILL_MODE GetRasterizerFillMode(RasterizerFillMode rasterizerFillMode);

	bool GenerateDepthStencilStateDesc(DepthStencilDesc DSDesc, DX11PipelineStateObject* PSO);
	bool GenerateBlendStateDesc(BlendDesc blendDesc, DX11PipelineStateObject* PSO);
	bool GenerateRasterizerStateDesc(RasterizerDesc rasterizerDesc, DX11PipelineStateObject* PSO);
	bool GenerateViewportStateDesc(ViewportDesc viewportDesc, DX11PipelineStateObject* PSO);

	bool LoadShaderFile(ID3D10Blob** rhs, ShaderStage shaderStage, const ShaderFilePath& shaderFilePath);
}
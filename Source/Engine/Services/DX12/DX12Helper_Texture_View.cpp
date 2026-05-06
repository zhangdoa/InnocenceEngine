#include "DX12Helper_Texture.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

using namespace Inno;

D3D12_SHADER_RESOURCE_VIEW_DESC DX12Helper::GetSRVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc, uint32_t mostDetailedMip)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC l_result = {};
	l_result.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_result.Format = DXGI_FORMAT_R32_FLOAT;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	else
	{
		l_result.Format = D3D12TextureDesc.Format;
	}

	// Calculate remaining mip levels from mostDetailedMip to end
	uint32_t totalMipLevels = GetTextureMipLevels(textureDesc);
	uint32_t remainingMipLevels = (mostDetailedMip < totalMipLevels) ? (totalMipLevels - mostDetailedMip) : 1;

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MostDetailedMip = mostDetailedMip;
		l_result.Texture1D.MipLevels = remainingMipLevels;
		break;
	case TextureSampler::Sampler2D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MostDetailedMip = mostDetailedMip;
		l_result.Texture2D.MipLevels = remainingMipLevels;
		break;
	case TextureSampler::Sampler3D:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MostDetailedMip = mostDetailedMip;
		l_result.Texture3D.MipLevels = remainingMipLevels;
		break;
	case TextureSampler::Sampler1DArray:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MostDetailedMip = mostDetailedMip;
		l_result.Texture1DArray.MipLevels = remainingMipLevels;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MostDetailedMip = mostDetailedMip;
		l_result.Texture2DArray.MipLevels = remainingMipLevels;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		l_result.TextureCube.MostDetailedMip = mostDetailedMip;
		l_result.TextureCube.MipLevels = remainingMipLevels;
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC DX12Helper::GetUAVDesc(TextureDesc textureDesc, D3D12_RESOURCE_DESC D3D12TextureDesc, uint32_t mipSlice)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_result = {};

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = mipSlice;
		break;
	case TextureSampler::Sampler2D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = mipSlice;
		break;
	case TextureSampler::Sampler3D:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = mipSlice;
		l_result.Texture3D.WSize = textureDesc.DepthOrArraySize / (uint32_t)std::pow(2, mipSlice);
		break;
	case TextureSampler::Sampler1DArray:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = mipSlice;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = mipSlice;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = mipSlice;
		l_result.Texture2DArray.ArraySize = 6;
		Log(Verbose, "Use 2D texture array for UAV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_RENDER_TARGET_VIEW_DESC DX12Helper::GetRTVDesc(TextureDesc textureDesc)
{
	D3D12_RENDER_TARGET_VIEW_DESC l_result = {};

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
		break;
	case TextureSampler::Sampler2D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
		break;
	case TextureSampler::Sampler3D:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MipSlice = 0;
		l_result.Texture3D.WSize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler1DArray:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = 0;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = 6;
		Log(Verbose, "Use 2D texture array for RTV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

D3D12_DEPTH_STENCIL_VIEW_DESC DX12Helper::GetDSVDesc(TextureDesc textureDesc, bool stencilEnable)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC l_result = {};

	if (stencilEnable)
	{
		l_result.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		l_result.Format = DXGI_FORMAT_D32_FLOAT;
	}

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
		break;
	case TextureSampler::Sampler2D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MipSlice = 0;
		break;
	case TextureSampler::Sampler3D:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		Log(Verbose, "Use 2D texture array for DSV of 3D texture.");
		break;
	case TextureSampler::Sampler1DArray:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		l_result.Texture1DArray.MipSlice = 0;
		l_result.Texture1DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		l_result.Texture2DArray.MipSlice = 0;
		l_result.Texture2DArray.ArraySize = 6;
		Log(Verbose, "Use 2D texture array for DSV of cubemap.");
		break;
	default:
		break;
	}

	return l_result;
}

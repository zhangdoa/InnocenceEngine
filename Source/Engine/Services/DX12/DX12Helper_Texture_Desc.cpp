#include "DX12Helper_Texture.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

using namespace Inno;

D3D12_RESOURCE_DESC DX12Helper::GetDX12TextureDesc(TextureDesc textureDesc)
{
	D3D12_RESOURCE_DESC l_result = {};

	l_result.Width = textureDesc.Width;
	l_result.Height = textureDesc.Height;
	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result.DepthOrArraySize = 1;
		break;
	case TextureSampler::Sampler2D:
		l_result.DepthOrArraySize = 1;
		break;
	case TextureSampler::Sampler3D:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler1DArray:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result.DepthOrArraySize = textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result.DepthOrArraySize = 6;
		break;
	default:
		break;
	}

	if (textureDesc.CPUAccessibility != Accessibility::Immutable)
	{
		l_result.MipLevels = 1;
		l_result.Format = DXGI_FORMAT_UNKNOWN;
		l_result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		l_result.SampleDesc.Count = 1;
		l_result.SampleDesc.Quality = 0;
		l_result.Flags = D3D12_RESOURCE_FLAG_NONE;
		l_result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	}
	else
	{
		l_result.MipLevels = GetTextureMipLevels(textureDesc);
		l_result.Format = GetTextureFormat(textureDesc);
		l_result.SampleDesc.Count = 1;
		l_result.SampleDesc.Quality = 0;
		l_result.Dimension = GetTextureDimension(textureDesc);
		l_result.Flags = GetTextureBindFlags(textureDesc);

		if (textureDesc.UseSharedHandle)
		{
			l_result.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		}
	}

	return l_result;
}

D3D12_RESOURCE_DIMENSION DX12Helper::GetTextureDimension(TextureDesc textureDesc)
{
	D3D12_RESOURCE_DIMENSION l_result;

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case TextureSampler::Sampler2D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case TextureSampler::Sampler3D: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		break;
	case TextureSampler::Sampler1DArray: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case TextureSampler::Sampler2DArray: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case TextureSampler::SamplerCubemap: l_result = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	default:
		break;
	}

	return l_result;
}

uint32_t DX12Helper::GetTextureMipLevels(TextureDesc textureDesc)
{
	if (textureDesc.MipLevels == 1 || textureDesc.PixelDataType == TexturePixelDataType::Compressed)
	{
		return 1;
	}

	uint32_t maxDimension = std::max(textureDesc.Width, textureDesc.Height);
	if (textureDesc.Sampler == TextureSampler::Sampler3D)
	{
		maxDimension = std::max(maxDimension, textureDesc.DepthOrArraySize);
	}

	if (maxDimension == 0)
	{
		Log(Error, "Invalid texture dimensions (Width: ", textureDesc.Width, ", Height: ", textureDesc.Height, ", Depth: ", textureDesc.DepthOrArraySize, ")");
		return 1;
	}

	uint32_t mipLevels = 1 + static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(maxDimension))));

	// Cap at 5 — finer mips are wasted on the engine's sampling distance.
	const uint32_t MAX_MIP_LEVELS = 5;
	mipLevels = std::min(mipLevels, MAX_MIP_LEVELS);

	return mipLevels;
}

D3D12_RESOURCE_FLAGS DX12Helper::GetTextureBindFlags(TextureDesc textureDesc)
{
	// BC formats don't support UAV in D3D12
	if (textureDesc.PixelDataType == TexturePixelDataType::Compressed)
		return D3D12_RESOURCE_FLAG_NONE;

	D3D12_RESOURCE_FLAGS l_result = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	if (textureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	else if (textureDesc.Usage == TextureUsage::ComputeOnly)
	{
		// ComputeOnly only needs UAV access, already set by default
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}

	return l_result;
}

#include "DX12Helper_Texture.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"
#include "DX12RenderingServer.h"

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
	}

	return l_result;
}

DXGI_FORMAT DX12Helper::GetTextureFormat(TextureDesc textureDesc)
{
	DXGI_FORMAT l_internalFormat = DXGI_FORMAT_UNKNOWN;

	if (textureDesc.IsSRGB)
	{
		l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_internalFormat = DXGI_FORMAT_R32_TYPELESS;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_internalFormat = DXGI_FORMAT_R24G8_TYPELESS;
	}
	else
	{
		if (textureDesc.PixelDataType == TexturePixelDataType::UByte)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SByte)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UShort)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SShort)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			default: break;
			}
		}
		if (textureDesc.PixelDataType == TexturePixelDataType::UInt8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			default: break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
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

D3D12_FILTER DX12Helper::GetFilterMode(TextureFilterMethod minFilterMethod, TextureFilterMethod magFilterMethod)
{
	D3D12_FILTER l_result;

	if (minFilterMethod == TextureFilterMethod::Nearest)
	{
		if (magFilterMethod == TextureFilterMethod::Nearest)
		{
			l_result = D3D12_FILTER_MIN_MAG_MIP_POINT;
		}
		else
		{
			l_result = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		}
	}
	else
	{
		if (magFilterMethod == TextureFilterMethod::Nearest)
		{
			l_result = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		}
		else
		{
			l_result = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		}
	}

	return l_result;
}

D3D12_TEXTURE_ADDRESS_MODE DX12Helper::GetWrapMode(TextureWrapMethod textureWrapMethod)
{
	D3D12_TEXTURE_ADDRESS_MODE l_result;

	switch (textureWrapMethod)
	{
	case TextureWrapMethod::Edge: l_result = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		break;
	case TextureWrapMethod::Repeat: l_result = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		break;
	case TextureWrapMethod::Border: l_result = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		break;
	default:
		break;
	}

	return l_result;
}

uint32_t DX12Helper::GetTextureMipLevels(TextureDesc textureDesc)
{
	if (!textureDesc.UseMipMap)
	{
		return 1;
	}

	// Calculate mip levels based on texture dimensions
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

	// Standard mip level calculation: 1 + floor(log2(max_dimension))
	uint32_t mipLevels = 1 + static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(maxDimension))));
	
	// Limit to maximum 5 mip levels - we don't need to go down to 1x1
	const uint32_t MAX_MIP_LEVELS = 5;
	mipLevels = std::min(mipLevels, MAX_MIP_LEVELS);

	return mipLevels;
}

D3D12_RESOURCE_FLAGS DX12Helper::GetTextureBindFlags(TextureDesc textureDesc)
{
	D3D12_RESOURCE_FLAGS l_result = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	if (textureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
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

uint32_t DX12Helper::GetTexturePixelDataSize(TextureDesc textureDesc)
{
	uint32_t l_singlePixelSize;

	switch (textureDesc.PixelDataType)
	{
	case TexturePixelDataType::UByte:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SByte:l_singlePixelSize = 1; break;
	case TexturePixelDataType::UShort:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SShort:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UInt8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SInt8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::UInt16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SInt16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UInt32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::SInt32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::Float16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::Float32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::Double:l_singlePixelSize = 8; break;
	}

	uint32_t l_channelSize;
	switch (textureDesc.PixelDataFormat)
	{
	case TexturePixelDataFormat::R:l_channelSize = 1; break;
	case TexturePixelDataFormat::RG:l_channelSize = 2; break;
	case TexturePixelDataFormat::RGB:l_channelSize = 3; break;
	case TexturePixelDataFormat::RGBA:l_channelSize = 4; break;
	case TexturePixelDataFormat::Depth:l_channelSize = 1; break;
	case TexturePixelDataFormat::DepthStencil:l_channelSize = 1; break;
	}

	return l_singlePixelSize * l_channelSize;
}

D3D12_RESOURCE_STATES DX12Helper::GetTextureWriteState(TextureDesc textureDesc)
{
	D3D12_RESOURCE_STATES l_result;
	if (textureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment
		|| textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}
	else
	{
		l_result = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		if (textureDesc.UseMipMap)
		{
			l_result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
	}

	return l_result;
}

D3D12_RESOURCE_STATES DX12Helper::GetTextureReadState(TextureDesc textureDesc)
{
	D3D12_RESOURCE_STATES l_result = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_SOURCE;

	if (textureDesc.Usage == TextureUsage::DepthAttachment
		|| textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result |= D3D12_RESOURCE_STATE_DEPTH_READ;
	}

	return l_result;
}

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

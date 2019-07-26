#include "DX11Helper.h"
#include "../../Core/InnoLogger.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

auto m_shaderRelativePath = L"Res//Shaders//DX11//";

D3D11_TEXTURE_DESC DX11Helper::GetDX11TextureDataDesc(TextureDataDesc textureDataDesc)
{
	D3D11_TEXTURE_DESC l_result = {};

	l_result.Width = textureDataDesc.width;
	l_result.Height = textureDataDesc.height;
	l_result.Depth = textureDataDesc.depth;
	l_result.ArraySize = 1;
	l_result.MipLevels = GetTextureMipLevels(textureDataDesc);
	l_result.Format = GetTextureFormat(textureDataDesc);
	l_result.SampleDesc.Count = 1;
	l_result.Usage = D3D11_USAGE_DEFAULT;
	l_result.BindFlags = GetTextureBindFlags(textureDataDesc);
	l_result.CPUAccessFlags = 0;

	if (textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_result.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	else
	{
		l_result.MiscFlags = 0;
	}

	return l_result;
}

DXGI_FORMAT DX11Helper::GetTextureFormat(TextureDataDesc textureDataDesc)
{
	DXGI_FORMAT l_internalFormat = DXGI_FORMAT_UNKNOWN;

	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_internalFormat = DXGI_FORMAT_R32_TYPELESS;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_internalFormat = DXGI_FORMAT_R24G8_TYPELESS;
	}
	else
	{
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			default: break;
			}
		}
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R8_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R8G8_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R16_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R16G16_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = DXGI_FORMAT_R32_FLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = DXGI_FORMAT_R32G32_FLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
}

unsigned int DX11Helper::GetTextureMipLevels(TextureDataDesc textureDataDesc)
{
	unsigned int textureMipLevels = 1;
	if (textureDataDesc.magFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		textureMipLevels = 0;
	}

	return textureMipLevels;
}

unsigned int DX11Helper::GetTextureBindFlags(TextureDataDesc textureDataDesc)
{
	unsigned int textureBindFlags = 0;
	if (textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	}
	else if (textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	}
	else
	{
		textureBindFlags = D3D11_BIND_SHADER_RESOURCE;
	}

	return textureBindFlags;
}

D3D11_TEXTURE1D_DESC DX11Helper::Get1DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc)
{
	D3D11_TEXTURE1D_DESC l_result = {};

	l_result.Width = textureDataDesc.Width;
	l_result.MipLevels = textureDataDesc.MipLevels;
	l_result.ArraySize = textureDataDesc.ArraySize;
	l_result.Format = textureDataDesc.Format;
	l_result.Usage = textureDataDesc.Usage;
	l_result.BindFlags = textureDataDesc.BindFlags;
	l_result.CPUAccessFlags = textureDataDesc.CPUAccessFlags;
	l_result.MiscFlags = textureDataDesc.MiscFlags;

	return l_result;
}

D3D11_TEXTURE2D_DESC DX11Helper::Get2DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc)
{
	D3D11_TEXTURE2D_DESC l_result = {};

	l_result.Width = textureDataDesc.Width;
	l_result.Height = textureDataDesc.Height;
	l_result.MipLevels = textureDataDesc.MipLevels;
	l_result.ArraySize = textureDataDesc.ArraySize;
	l_result.Format = textureDataDesc.Format;
	l_result.SampleDesc = textureDataDesc.SampleDesc;
	l_result.Usage = textureDataDesc.Usage;
	l_result.BindFlags = textureDataDesc.BindFlags;
	l_result.CPUAccessFlags = textureDataDesc.CPUAccessFlags;
	l_result.MiscFlags = textureDataDesc.MiscFlags;

	return l_result;
}

D3D11_TEXTURE3D_DESC DX11Helper::Get3DTextureDataDesc(D3D11_TEXTURE_DESC textureDataDesc)
{
	D3D11_TEXTURE3D_DESC l_result = {};

	l_result.Width = textureDataDesc.Width;
	l_result.Height = textureDataDesc.Height;
	l_result.Depth = textureDataDesc.Depth;
	l_result.MipLevels = textureDataDesc.MipLevels;
	l_result.Format = textureDataDesc.Format;
	l_result.Usage = textureDataDesc.Usage;
	l_result.BindFlags = textureDataDesc.BindFlags;
	l_result.CPUAccessFlags = textureDataDesc.CPUAccessFlags;
	l_result.MiscFlags = textureDataDesc.MiscFlags;

	return l_result;
}

UINT GetMipLevels(TextureDataDesc textureDataDesc)
{
	if (textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
		|| textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
		|| textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
		|| textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

D3D11_SHADER_RESOURCE_VIEW_DESC DX11Helper::GetSRVDesc(TextureDataDesc textureDataDesc, D3D11_TEXTURE_DESC D3D11TextureDesc)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC l_result = {};

	if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_result.Format = DXGI_FORMAT_R32_FLOAT;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_result.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	else
	{
		l_result.Format = D3D11TextureDesc.Format;
	}

	if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_1D)
	{
		l_result.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MostDetailedMip = 0;
		l_result.Texture1D.MipLevels = GetMipLevels(textureDataDesc);
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		l_result.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		l_result.Texture2D.MostDetailedMip = 0;
		l_result.Texture2D.MipLevels = GetMipLevels(textureDataDesc);
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_3D)
	{
		l_result.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		l_result.Texture3D.MostDetailedMip = 0;
		l_result.Texture3D.MipLevels = GetMipLevels(textureDataDesc);
	}
	else
	{
		// @TODO: Cubemap support
	}

	return l_result;
}

D3D11_UNORDERED_ACCESS_VIEW_DESC DX11Helper::GetUAVDesc(TextureDataDesc textureDataDesc, D3D11_TEXTURE_DESC D3D11TextureDesc)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC l_result = {};

	if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_1D)
	{
		l_result.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
		l_result.Texture1D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		l_result.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		l_result.Texture1D.MipSlice = 0;
	}
	else if (textureDataDesc.samplerType == TextureSamplerType::SAMPLER_3D)
	{
		l_result.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
		l_result.Texture1D.MipSlice = 0;
	}
	else
	{
		// @TODO: Cubemap support
	}

	return l_result;
}
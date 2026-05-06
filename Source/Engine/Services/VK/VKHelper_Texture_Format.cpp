#include "VKHelper_Texture.h"

using namespace Inno;

VkFormat VKHelper::GetTextureFormat(TextureDesc textureDesc)
{
	VkFormat l_internalFormat = VK_FORMAT_R8_UNORM;

	if (textureDesc.IsSRGB)
	{
		l_internalFormat = VK_FORMAT_R8G8B8A8_SRGB;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D32_SFLOAT;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		if (textureDesc.PixelDataType == TexturePixelDataType::UByte)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_UNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_UNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_UNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SByte)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_SNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_SNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_SNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UShort)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_UNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_UNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SShort)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_SNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_SNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_UINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_UINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UINT;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_UINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_SINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_SINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SINT;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_SINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_UINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_UINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_SINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_SINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R32_UINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R32G32_UINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R32G32B32A32_UINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R32G32B32A32_UINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R32_SINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R32G32_SINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_SFLOAT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_SFLOAT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R32_SFLOAT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R32G32_SFLOAT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			default:
				break;
			}
		}
	}

	return l_internalFormat;
}

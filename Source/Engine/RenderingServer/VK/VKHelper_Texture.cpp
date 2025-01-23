#include "VKHelper_Texture.h"

#include "../../Engine.h"

using namespace Inno;

VKTextureDesc VKHelper::GetVKTextureDesc(TextureDesc textureDesc)
{
	VKTextureDesc l_result;

	l_result.imageType = GetImageType(textureDesc.Sampler);
	l_result.imageViewType = GetImageViewType(textureDesc.Sampler);
	l_result.imageUsageFlags = GetImageUsageFlags(textureDesc.Usage);
	l_result.format = GetTextureFormat(textureDesc);
	l_result.imageSize = GetImageSize(textureDesc);
	l_result.aspectFlags = GetImageAspectFlags(textureDesc.Usage);

	return l_result;
}

VkImageType VKHelper::GetImageType(TextureSampler textureSampler)
{
	VkImageType l_result;

	switch (textureSampler)
	{
	case TextureSampler::Sampler1D:
		l_result = VkImageType::VK_IMAGE_TYPE_1D;
		break;
	case TextureSampler::Sampler2D:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	case TextureSampler::Sampler3D:
		l_result = VkImageType::VK_IMAGE_TYPE_3D;
		break;
	case TextureSampler::Sampler1DArray:
		l_result = VkImageType::VK_IMAGE_TYPE_1D;
		break;
	case TextureSampler::Sampler2DArray:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	case TextureSampler::SamplerCubemap:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	default:
		break;
	}

	return l_result;
}

VkImageViewType VKHelper::GetImageViewType(TextureSampler textureSampler)
{
	VkImageViewType l_result;

	switch (textureSampler)
	{
	case TextureSampler::Sampler1D:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_1D;
		break;
	case TextureSampler::Sampler2D:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
		break;
	case TextureSampler::Sampler3D:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_3D;
		break;
	case TextureSampler::Sampler1DArray:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		break;
	case TextureSampler::Sampler2DArray:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		break;
	case TextureSampler::SamplerCubemap:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
		break;
	default:
		break;
	}

	return l_result;
}

VkImageUsageFlags VKHelper::GetImageUsageFlags(TextureUsage textureUsage)
{
	VkImageUsageFlags l_result = VK_IMAGE_USAGE_SAMPLED_BIT;

	if (textureUsage == TextureUsage::ColorAttachment)
	{
		l_result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	else if (textureUsage == TextureUsage::DepthAttachment)
	{
		l_result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else if (textureUsage == TextureUsage::DepthStencilAttachment)
	{
		l_result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else
	{
		l_result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}

	return l_result;
}

VkSamplerAddressMode VKHelper::GetSamplerAddressMode(TextureWrapMethod textureWrapMethod)
{
	VkSamplerAddressMode l_result;

	switch (textureWrapMethod)
	{
	case TextureWrapMethod::Edge:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case TextureWrapMethod::Repeat:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case TextureWrapMethod::Border:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	default:
		break;
	}

	return l_result;
}

VkFilter VKHelper::GetFilter(TextureFilterMethod textureFilterMethod)
{
	VkFilter l_result;

	switch (textureFilterMethod)
	{
	case TextureFilterMethod::Nearest:
		l_result = VkFilter::VK_FILTER_NEAREST;
		break;
	case TextureFilterMethod::Linear:
		l_result = VkFilter::VK_FILTER_LINEAR;
		break;
	default:
		break;
	}

	return l_result;
}

VkSamplerMipmapMode VKHelper::GetSamplerMipmapMode(TextureFilterMethod minFilterMethod)
{
	VkSamplerMipmapMode l_result;

	switch (minFilterMethod)
	{
	case TextureFilterMethod::Nearest:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case TextureFilterMethod::Linear:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	default:
		break;
	}

	return l_result;
}

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

VkDeviceSize VKHelper::GetImageSize(TextureDesc textureDesc)
{
	VkDeviceSize l_result;

	VkDeviceSize l_singlePixelSize;

	switch (textureDesc.PixelDataType)
	{
	case TexturePixelDataType::UByte:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::SByte:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::UShort:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::SShort:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::UInt8:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::SInt8:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::UInt16:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::SInt16:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::UInt32:
		l_singlePixelSize = 4;
		break;
	case TexturePixelDataType::SInt32:
		l_singlePixelSize = 4;
		break;
	case TexturePixelDataType::Float16:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::Float32:
		l_singlePixelSize = 4;
		break;
	case TexturePixelDataType::Double:
		l_singlePixelSize = 8;
		break;
	}

	VkDeviceSize l_channelSize;

	switch (textureDesc.PixelDataFormat)
	{
	case TexturePixelDataFormat::R:
		l_channelSize = 1;
		break;
	case TexturePixelDataFormat::RG:
		l_channelSize = 2;
		break;
	case TexturePixelDataFormat::RGB:
		l_channelSize = 3;
		break;
	case TexturePixelDataFormat::RGBA:
		l_channelSize = 4;
		break;
	case TexturePixelDataFormat::Depth:
		l_channelSize = 1;
		break;
	case TexturePixelDataFormat::DepthStencil:
		l_channelSize = 1;
		break;
	}

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result = textureDesc.Width * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler2D:
		l_result = textureDesc.Width * textureDesc.Height * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler3D:
		l_result = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler1DArray:
		l_result = textureDesc.Width * textureDesc.DepthOrArraySize * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result = textureDesc.Width * textureDesc.Height * 6 * l_singlePixelSize * l_channelSize;
		break;
	default:
		break;
	}

	return l_result;
}

VkImageAspectFlagBits VKHelper::GetImageAspectFlags(TextureUsage textureUsage)
{
	VkImageAspectFlagBits l_result;

	if (textureUsage == TextureUsage::DepthAttachment)
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (textureUsage == TextureUsage::DepthStencilAttachment)
	{
		l_result = VkImageAspectFlagBits(VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	else
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	}

	return l_result;
}

VkImageCreateInfo VKHelper::GetImageCreateInfo(TextureDesc textureDesc, VKTextureDesc vKTextureDesc)
{
	VkImageCreateInfo l_result = {};

	l_result.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

	if (textureDesc.Sampler == TextureSampler::Sampler2DArray)
	{
		//l_result.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
	}
	else if (textureDesc.Sampler == TextureSampler::SamplerCubemap)
	{
		l_result.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	l_result.imageType = vKTextureDesc.imageType;
	l_result.extent.width = textureDesc.Width;
	l_result.extent.height = textureDesc.Height;
	if (textureDesc.Sampler == TextureSampler::Sampler3D)
	{
		l_result.extent.depth = textureDesc.DepthOrArraySize;
	}
	else
	{
		l_result.extent.depth = 1;
	}
	l_result.mipLevels = 1;
	if (textureDesc.Sampler == TextureSampler::Sampler1DArray ||
		textureDesc.Sampler == TextureSampler::Sampler2DArray)
	{
		l_result.arrayLayers = textureDesc.DepthOrArraySize;
	}
	else if (textureDesc.Sampler == TextureSampler::SamplerCubemap)
	{
		l_result.arrayLayers = 6;
	}
	else
	{
		l_result.arrayLayers = 1;
	}
	l_result.format = vKTextureDesc.format;
	l_result.tiling = VK_IMAGE_TILING_OPTIMAL;
	l_result.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	l_result.samples = VK_SAMPLE_COUNT_1_BIT;
	l_result.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	l_result.usage = vKTextureDesc.imageUsageFlags;

	return l_result;
}

VkImageLayout VKHelper::GetTextureWriteImageLayout(TextureDesc textureDesc)
{
	VkImageLayout l_result = VK_IMAGE_LAYOUT_GENERAL;
	if (textureDesc.Usage == TextureUsage::ColorAttachment)
	{
		l_result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_result = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	return l_result;
}

VkImageLayout VKHelper::GetTextureReadImageLayout(TextureDesc textureDesc)
{
	VkImageLayout l_result = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_result = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}

	return l_result;
}

VkAccessFlagBits VKHelper::GetAccessMask(const VkImageLayout& imageLayout)
{
	if (imageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		return VK_ACCESS_TRANSFER_READ_BIT;
	}
	if (imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	if(imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
	 	return VK_ACCESS_SHADER_READ_BIT;
	}
	if(imageLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
	 	return VkAccessFlagBits(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
	}
	if(imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
	 	return VkAccessFlagBits(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	}
	if(imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
	 	return VkAccessFlagBits(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	}

	return VK_ACCESS_NONE;
}
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

VkImageUsageFlags VKHelper::GetImageUsageFlags(TextureUsage textureUsage)
{
	VkImageUsageFlags l_result = VK_IMAGE_USAGE_SAMPLED_BIT;

	if (textureUsage == TextureUsage::ColorAttachment)
	{
		l_result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	else if (textureUsage == TextureUsage::ComputeOnly)
	{
		l_result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
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

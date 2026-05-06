#include "VKHelper_Texture.h"

using namespace Inno;

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

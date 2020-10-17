#pragma once
#include "../Common/InnoType.h"
#include "vulkan/vulkan.h"
#include "TextureDataComponent.h"

namespace Inno
{
	struct VKTextureDesc
	{
		VkImageType imageType;
		VkImageViewType imageViewType;
		VkImageUsageFlags imageUsageFlags;
		VkFormat format;
		VkDeviceSize imageSize;
		VkBorderColor boarderColor;
		VkImageAspectFlags aspectFlags;
	};

	class VKTextureDataComponent : public TextureDataComponent
	{
	public:
		VkImage m_image;
		VkDeviceMemory m_imageMemory;
		VkImageView m_imageView;
		VKTextureDesc m_VKTextureDesc = {};
		VkImageCreateInfo m_ImageCreateInfo = {};
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};
}
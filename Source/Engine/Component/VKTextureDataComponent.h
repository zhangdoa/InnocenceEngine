#pragma once
#include "../Common/InnoType.h"
#include "vulkan/vulkan.h"
#include "TextureDataComponent.h"

struct VKTextureDataDesc
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
	VkImageView m_imageView;
	VKTextureDataDesc m_VKTextureDataDesc = {};
	VkImageCreateInfo m_ImageCreateInfo = {};
	VkDescriptorImageInfo m_DescriptorImageInfo = {};
};

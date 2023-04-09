#pragma once
#include "../Common/InnoType.h"
#include "vulkan/vulkan.h"
#include "TextureComponent.h"

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

	class VKTextureComponent : public TextureComponent
	{
	public:
		VkImage m_image;
		VkDeviceMemory m_imageMemory;
		VkImageView m_imageView;
		VKTextureDesc m_VKTextureDesc = {};
		VkImageCreateInfo m_ImageCreateInfo = {};
		VkImageLayout m_WriteImageLayout;
		VkImageLayout m_ReadImageLayout;
		VkImageLayout m_CurrentImageLayout;
	};
}
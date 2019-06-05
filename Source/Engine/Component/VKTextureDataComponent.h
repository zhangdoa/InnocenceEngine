#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"
#include "TextureDataComponent.h"

struct VkTextureDataDesc
{
	VkImageType imageType;
	VkSamplerAddressMode samplerAddressMode;
	VkSamplerMipmapMode minFilterParam;
	VkSamplerMipmapMode magFilterParam;
	VkFormat format;
	VkDeviceSize imageSize;
	VkBorderColor boarderColor;
	VkImageAspectFlags aspectFlags;
};

class VKTextureDataComponent : public TextureDataComponent
{
public:
	VKTextureDataComponent() {};
	~VKTextureDataComponent() {};

	VkImage m_image;
	VkImageView m_imageView;

	VkTextureDataDesc m_VkTextureDataDesc = VkTextureDataDesc();
};

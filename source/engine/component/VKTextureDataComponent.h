#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"
#include "TextureDataComponent.h"

struct VkTextureDataDesc
{
	VkSamplerAddressMode textureWrapMethod;
	VkSamplerMipmapMode minFilterParam;
	VkSamplerMipmapMode magFilterParam;
	VkFormat internalFormat;
	VkBorderColor boarderColor;
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

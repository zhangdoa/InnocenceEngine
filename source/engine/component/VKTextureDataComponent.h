#pragma once
#include "../common/InnoType.h"

struct VkTextureDataDesc
{
	VkSamplerAddressMode textureWrapMethod;
	VkSamplerMipmapMode minFilterParam;
	VkSamplerMipmapMode magFilterParam;
	VkFormat internalFormat;
	VkBorderColor boarderColor;
	unsigned int width;
	unsigned int height;
};

class VKTextureDataComponent
{
public:
	VKTextureDataComponent() {};
	~VKTextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	VkImage m_image;
	VkImageView m_imageView;

	VkTextureDataDesc m_VkTextureDataDesc = VkTextureDataDesc();
};

#pragma once
#include "../common/InnoType.h"

struct VKTextureDataDesc
{
	VkSamplerAddressMode textureWrapMethod;
	VkSamplerMipmapMode minFilterParam;
	VkSamplerMipmapMode magFilterParam;
	VkFormat internalFormat;
	VkBorderColor boardColor;
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

	VKTextureDataDesc m_VKTextureDataDesc = VKTextureDataDesc();
};

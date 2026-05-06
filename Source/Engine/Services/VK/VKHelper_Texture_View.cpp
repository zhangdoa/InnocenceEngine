#include "VKHelper_Texture.h"

using namespace Inno;

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

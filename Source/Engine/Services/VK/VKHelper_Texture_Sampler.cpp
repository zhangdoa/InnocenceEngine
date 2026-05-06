#include "VKHelper_Texture.h"

using namespace Inno;

VkSamplerAddressMode VKHelper::GetSamplerAddressMode(TextureWrapMethod textureWrapMethod)
{
	VkSamplerAddressMode l_result;

	switch (textureWrapMethod)
	{
	case TextureWrapMethod::Edge:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case TextureWrapMethod::Repeat:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case TextureWrapMethod::Border:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	default:
		break;
	}

	return l_result;
}

VkFilter VKHelper::GetFilter(TextureFilterMethod textureFilterMethod)
{
	VkFilter l_result;

	switch (textureFilterMethod)
	{
	case TextureFilterMethod::Nearest:
		l_result = VkFilter::VK_FILTER_NEAREST;
		break;
	case TextureFilterMethod::Linear:
		l_result = VkFilter::VK_FILTER_LINEAR;
		break;
	default:
		break;
	}

	return l_result;
}

VkSamplerMipmapMode VKHelper::GetSamplerMipmapMode(TextureFilterMethod minFilterMethod)
{
	VkSamplerMipmapMode l_result;

	switch (minFilterMethod)
	{
	case TextureFilterMethod::Nearest:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case TextureFilterMethod::Linear:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	default:
		break;
	}

	return l_result;
}

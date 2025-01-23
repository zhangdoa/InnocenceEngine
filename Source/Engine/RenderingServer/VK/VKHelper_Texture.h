#pragma once
#include "../../Common/STL17.h"
#include "../../Common/LogService.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../IRenderingServer.h"

namespace Inno
{
	namespace VKHelper
	{
		VKTextureDesc GetVKTextureDesc(TextureDesc textureDesc);
		VkImageType GetImageType(TextureSampler textureSampler);
		VkImageViewType GetImageViewType(TextureSampler textureSampler);
		VkImageUsageFlags GetImageUsageFlags(TextureUsage textureUsage);
		VkSamplerAddressMode GetSamplerAddressMode(TextureWrapMethod textureWrapMethod);
		VkFilter GetFilter(TextureFilterMethod textureFilterMethod);
		VkSamplerMipmapMode GetSamplerMipmapMode(TextureFilterMethod minFilterMethod);
		VkFormat GetTextureFormat(TextureDesc textureDesc);
		VkDeviceSize GetImageSize(TextureDesc textureDesc);
		VkImageAspectFlagBits GetImageAspectFlags(TextureUsage textureUsage);
		VkImageCreateInfo GetImageCreateInfo(TextureDesc textureDesc, VKTextureDesc vKTextureDesc);
		
		VkImageLayout GetTextureWriteImageLayout(TextureDesc textureDesc);
		VkImageLayout GetTextureReadImageLayout(TextureDesc textureDesc);
		VkAccessFlagBits GetAccessMask(const VkImageLayout& imageLayout);
	}
}
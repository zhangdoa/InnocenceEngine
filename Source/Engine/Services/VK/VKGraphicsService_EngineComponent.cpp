#include "VKGraphicsService.h"
#include "../../Common/Array.h"
#include "../GraphicsResourceService.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
#include "VKHelper_Texture.h"
#include "VKHelper_Pipeline.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

template <typename U, typename T>
bool VKGraphicsService::SetObjectName(U* owner, const T& rhs, VkObjectType objectType, const char* objectTypeSuffix)
{
	std::string l_Name;
	l_Name += "_";
	l_Name += objectTypeSuffix;

	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.pNext = nullptr;
	nameInfo.objectType = objectType;
	nameInfo.objectHandle = (uint64_t)rhs;
	nameInfo.pObjectName = l_Name.c_str();

	auto l_result = SetDebugUtilsObjectNameEXT(&nameInfo);
	if (l_result != VK_SUCCESS)
	{
		Log(Warning, "Can't name ", objectType, " with ", l_Name.c_str());
		return false;
	}
	return true;
}

bool VKGraphicsService::InitializeImpl(MeshAssetHandle handle, Inno::Array<Vertex> &vertices, Inno::Array<Index> &indices)
{
	return true;
}

void VKGraphicsService::ReleaseMeshGPUResourceImpl(MeshAssetHandle handle)
{
}

bool VKGraphicsService::InitializeImpl(TextureComponent *rhs, void *textureData)
{
	auto l_rhs = reinterpret_cast<VKTextureComponent *>(rhs);
	l_rhs->m_VKTextureDesc = GetVKTextureDesc(rhs->m_TextureDesc);
	l_rhs->m_ImageCreateInfo = GetImageCreateInfo(rhs->m_TextureDesc, l_rhs->m_VKTextureDesc);
	l_rhs->m_WriteImageLayout = GetTextureWriteImageLayout(l_rhs->m_TextureDesc);
	l_rhs->m_ReadImageLayout = GetTextureReadImageLayout(l_rhs->m_TextureDesc);
	if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_rhs->m_CurrentImageLayout = l_rhs->m_WriteImageLayout;
	}
	else
	{
		l_rhs->m_CurrentImageLayout = l_rhs->m_ReadImageLayout;
	}

	CreateImage(l_rhs->m_ImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, l_rhs->m_image, l_rhs->m_imageMemory);

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_rhs, l_rhs->m_image, VK_OBJECT_TYPE_IMAGE, "Image");
#endif //  INNO_DEBUG

	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;
	if (textureData != nullptr)
	{
		CreateHostStagingBuffer(l_rhs->m_VKTextureDesc.imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, l_stagingBuffer, l_stagingBufferMemory);

		void *l_mappedMemory;
		vkMapMemory(m_device, l_stagingBufferMemory, 0, l_rhs->m_VKTextureDesc.imageSize, 0, &l_mappedMemory);
		std::memcpy(l_mappedMemory, textureData, static_cast<size_t>(l_rhs->m_VKTextureDesc.imageSize));
		vkUnmapMemory(m_device, l_stagingBufferMemory);
	}

	VkCommandBuffer l_commandBuffer = OpenTemporaryCommandBuffer(m_globalCommandPool);

	if (textureData != nullptr)
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(l_commandBuffer, l_stagingBuffer, l_rhs->m_image, l_rhs->m_VKTextureDesc.aspectFlags, static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.width), static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.height));
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, l_rhs->m_CurrentImageLayout);
	}
	else
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, l_rhs->m_CurrentImageLayout);
	}

	CloseTemporaryCommandBuffer(m_globalCommandPool, m_graphicsQueue, l_commandBuffer);

	if (textureData != nullptr)
	{
		vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
		vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);
	}

	CreateImageView(l_rhs);

	l_rhs->m_GPUResourceType = GPUResourceType::Image;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	Log(Verbose, "VkImage ", l_rhs->m_image, " is initialized.");

	return true;
}

bool VKGraphicsService::InitializeImpl(SamplerComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKSamplerComponent *>(rhs);

	l_rhs->m_samplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	l_rhs->m_samplerCInfo.minFilter = GetFilter(l_rhs->m_SamplerDesc.m_MinFilterMethod);
	l_rhs->m_samplerCInfo.magFilter = GetFilter(l_rhs->m_SamplerDesc.m_MagFilterMethod);
	l_rhs->m_samplerCInfo.mipmapMode = GetSamplerMipmapMode(l_rhs->m_SamplerDesc.m_MinFilterMethod);
	l_rhs->m_samplerCInfo.addressModeU = GetSamplerAddressMode(l_rhs->m_SamplerDesc.m_WrapMethodU);
	l_rhs->m_samplerCInfo.addressModeV = GetSamplerAddressMode(l_rhs->m_SamplerDesc.m_WrapMethodV);
	l_rhs->m_samplerCInfo.addressModeW = GetSamplerAddressMode(l_rhs->m_SamplerDesc.m_WrapMethodW);
	l_rhs->m_samplerCInfo.mipLodBias = 0.0f;
	l_rhs->m_samplerCInfo.maxAnisotropy = float(l_rhs->m_SamplerDesc.m_MaxAnisotropy);
	l_rhs->m_samplerCInfo.compareOp = VkCompareOp::VK_COMPARE_OP_ALWAYS;
	l_rhs->m_samplerCInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
	l_rhs->m_samplerCInfo.minLod = l_rhs->m_SamplerDesc.m_MinLOD;
	l_rhs->m_samplerCInfo.maxLod = l_rhs->m_SamplerDesc.m_MaxLOD;

	VkSamplerCustomBorderColorCreateInfoEXT l_samplerCustomBorderColorCInfoEXT = {};
	l_samplerCustomBorderColorCInfoEXT.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[0] = l_rhs->m_SamplerDesc.m_BorderColor[0];
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[1] = l_rhs->m_SamplerDesc.m_BorderColor[1];
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[2] = l_rhs->m_SamplerDesc.m_BorderColor[2];
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[3] = l_rhs->m_SamplerDesc.m_BorderColor[3];
	l_samplerCustomBorderColorCInfoEXT.format = VK_FORMAT_UNDEFINED;

	l_rhs->m_samplerCInfo.pNext = &l_samplerCustomBorderColorCInfoEXT;

	if (vkCreateSampler(m_device, &l_rhs->m_samplerCInfo, nullptr, &l_rhs->m_sampler) != VK_SUCCESS)
	{
		Log(Error, "Failed to create sampler!");
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Sampler;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VKGraphicsService::InitializeImpl(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferComponent *>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	CreateHostStagingBuffer(l_rhs->m_TotalSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_HostStagingBuffer, l_rhs->m_HostStagingMemory);

	if (l_rhs->m_InitialData != nullptr)
	{
		CopyHostMemoryToDeviceMemory(l_rhs->m_InitialData, l_rhs->m_TotalSize, l_rhs->m_HostStagingMemory);
	}

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		if (l_rhs->m_CPUAccessibility == Accessibility::Immutable || l_rhs->m_CPUAccessibility == Accessibility::WriteOnly)
		{
			CreateDeviceLocalBuffer(l_rhs->m_TotalSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_DeviceLocalBuffer, l_rhs->m_DeviceLocalMemory);
			if (l_rhs->m_InitialData != nullptr)
			{
				CopyBuffer(m_globalCommandPool, m_graphicsQueue, l_rhs->m_HostStagingBuffer, l_rhs->m_DeviceLocalBuffer, l_rhs->m_TotalSize);
			}
		}
		else
		{
			Log(Warning, "Not support CPU-readable default heap GPU buffer currently.");
		}
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Buffer;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VKGraphicsService::UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferComponent*>(gpuBuffer);
	if (!l_rhs->m_DeviceLocalMemory)
		return true;

	CopyBuffer(m_globalCommandPool, m_graphicsQueue, l_rhs->m_HostStagingBuffer, l_rhs->m_DeviceLocalBuffer, l_rhs->m_TotalSize);

	return true;
}

bool VKGraphicsService::CreateImageView(VKTextureComponent *VKTextureComp)
{
	VkImageViewCreateInfo l_viewCInfo = {};
	l_viewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	l_viewCInfo.image = VKTextureComp->m_image;
	l_viewCInfo.viewType = VKTextureComp->m_VKTextureDesc.imageViewType;
	l_viewCInfo.format = VKTextureComp->m_VKTextureDesc.format;
	l_viewCInfo.subresourceRange.aspectMask = VKTextureComp->m_VKTextureDesc.aspectFlags;
	l_viewCInfo.subresourceRange.baseMipLevel = 0;
	l_viewCInfo.subresourceRange.levelCount = 1;
	l_viewCInfo.subresourceRange.baseArrayLayer = 0;
	if (VKTextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler1DArray ||
		VKTextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler2DArray)
	{
		l_viewCInfo.subresourceRange.layerCount = VKTextureComp->m_TextureDesc.DepthOrArraySize;
	}
	else if (VKTextureComp->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap)
	{
		l_viewCInfo.subresourceRange.layerCount = 6;
	}
	else
	{
		l_viewCInfo.subresourceRange.layerCount = 1;
	}

	if (vkCreateImageView(m_device, &l_viewCInfo, nullptr, &VKTextureComp->m_imageView) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkImageView!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKTextureComp, VKTextureComp->m_imageView, VK_OBJECT_TYPE_IMAGE_VIEW, "ImageView");
#endif //  INNO_DEBUG

	Log(Verbose, "VkImageView ", VKTextureComp->m_imageView, " is initialized.");

	return true;
}

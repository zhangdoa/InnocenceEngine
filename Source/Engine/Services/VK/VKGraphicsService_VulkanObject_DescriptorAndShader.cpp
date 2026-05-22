#include "VKGraphicsService.h"
#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
#include "VKHelper_Texture.h"
#include "VKHelper_Pipeline.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

namespace Inno
{
	namespace VKHelper
	{
		const char *m_shaderRelativePath = "Shaders//SPIRV//";
	}
} // namespace Inno

bool VKGraphicsService::CreateDescriptorPool(VkDescriptorPoolSize *poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool &poolHandle)
{
	VkDescriptorPoolCreateInfo l_poolCInfo = {};
	l_poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	l_poolCInfo.poolSizeCount = poolSizeCount;
	l_poolCInfo.pPoolSizes = poolSize;
	l_poolCInfo.maxSets = maxSets;
	l_poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	if (vkCreateDescriptorPool(m_device, &l_poolCInfo, nullptr, &poolHandle) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkDescriptorPool!");
		return false;
	}

	Log(Verbose, "VkDescriptorPool has been created.");
	return true;
}

bool VKGraphicsService::CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding *setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout &setLayout)
{
	VkDescriptorSetLayoutCreateInfo l_layoutCInfo = {};
	l_layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	l_layoutCInfo.bindingCount = setLayoutBindingsCount;
	l_layoutCInfo.pBindings = setLayoutBindings;
	l_layoutCInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	if (vkCreateDescriptorSetLayout(m_device, &l_layoutCInfo, nullptr, &setLayout) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkDescriptorSetLayout!");
		return false;
	}

	Log(Verbose, "VkDescriptorSetLayout has been created.");
	return true;
}

bool VKGraphicsService::CreateDescriptorSets(VkDescriptorPool pool, const VkDescriptorSetLayout *setLayout, VkDescriptorSet &setHandle, uint32_t count)
{
	VkDescriptorSetAllocateInfo l_allocCInfo = {};
	l_allocCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	l_allocCInfo.descriptorPool = pool;
	l_allocCInfo.descriptorSetCount = count;
	l_allocCInfo.pSetLayouts = setLayout;

	if (vkAllocateDescriptorSets(m_device, &l_allocCInfo, &setHandle) != VK_SUCCESS)
	{
		Log(Error, "Failed to allocate VkDescriptorSet!");
		return false;
	}

	Log(Verbose, "VkDescriptorSet has been allocated.");
	return true;
}

bool VKGraphicsService::UpdateDescriptorSet(VkWriteDescriptorSet *writeDescriptorSets, uint32_t writeDescriptorSetsCount)
{
	vkUpdateDescriptorSets(
		m_device,
		writeDescriptorSetsCount,
		writeDescriptorSets,
		0,
		nullptr);

	Log(Verbose, "Write VkDescriptorSet has been updated.");
	return true;
}

VkWriteDescriptorSet VKGraphicsService::GetWriteDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
{
	VkWriteDescriptorSet l_result = {};
	l_result.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	l_result.dstBinding = dstBinding;
	l_result.dstArrayElement = 0;
	l_result.descriptorType = descriptorType;
	l_result.descriptorCount = 1;
	l_result.dstSet = descriptorSet;

	return l_result;
}

VkWriteDescriptorSet VKGraphicsService::GetWriteDescriptorSet(const VkDescriptorImageInfo &imageInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
{
	VkWriteDescriptorSet l_result = {};
	l_result.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	l_result.dstBinding = dstBinding;
	l_result.dstArrayElement = 0;
	l_result.descriptorType = descriptorType;
	l_result.descriptorCount = 1;
	l_result.pImageInfo = &imageInfo;
	l_result.dstSet = descriptorSet;

	return l_result;
}

VkWriteDescriptorSet VKGraphicsService::GetWriteDescriptorSet(const VkDescriptorBufferInfo &bufferInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
{
	VkWriteDescriptorSet l_result = {};
	l_result.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	l_result.dstBinding = dstBinding;
	l_result.dstArrayElement = 0;
	l_result.descriptorType = descriptorType;
	l_result.descriptorCount = 1;
	l_result.pBufferInfo = &bufferInfo;
	l_result.dstSet = descriptorSet;

	return l_result;
}

bool VKGraphicsService::CreateShaderModule(VkShaderModule &vkShaderModule, const ShaderFilePath &shaderFilePath)
{
	auto l_shaderFileName = m_shaderRelativePath + std::string(shaderFilePath.c_str()) + ".spv";
	auto l_shaderContent = g_Engine->Get<IOService>()->LoadFile(l_shaderFileName.c_str(), IOMode::Binary);

	VkShaderModuleCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	l_createInfo.codeSize = l_shaderContent.size();
	l_createInfo.pCode = reinterpret_cast<const uint32_t *>(l_shaderContent.data());

	if (vkCreateShaderModule(m_device, &l_createInfo, nullptr, &vkShaderModule) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkShaderModule for: ", shaderFilePath.c_str(), "!");
		return false;
	}

	Log(Verbose, "", shaderFilePath.c_str(), " has been loaded.");
	return true;
}

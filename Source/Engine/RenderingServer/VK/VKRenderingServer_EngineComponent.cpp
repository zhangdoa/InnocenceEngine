#include "VKRenderingServer.h"

#include "../CommonFunctionDefinationMacro.inl"

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
#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/EntityManager.h"

template <typename U, typename T>
bool VKRenderingServer::SetObjectName(U* owner, const T& rhs, VkObjectType objectType, const char* objectTypeSuffix)
{
	auto l_Name = std::string(owner->m_InstanceName.c_str());
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

bool VKRenderingServer::InitializeImpl(MeshComponent *rhs, std::vector<Vertex> &vertices, std::vector<Index> &indices)
{
	auto l_rhs = reinterpret_cast<VKMeshComponent *>(rhs);

	auto l_VBSize = sizeof(Vertex) * vertices.size();
	auto l_IBSize = sizeof(Index) * indices.size();

	CreateDeviceLocalBuffer(l_VBSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_VBO, l_rhs->m_VBMemory);
	CreateDeviceLocalBuffer(l_IBSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_IBO, l_rhs->m_IBMemory);

	InitializeDeviceLocalBuffer(&vertices[0], l_VBSize, l_rhs->m_VBO, l_rhs->m_VBMemory);
	Log(Verbose, "VBO ", l_rhs->m_VBO, " is initialized.");

	InitializeDeviceLocalBuffer(&indices[0], l_IBSize, l_rhs->m_IBO, l_rhs->m_IBMemory);
	Log(Verbose, "IBO ", l_rhs->m_IBO, " is initialized.");

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_rhs, l_rhs->m_VBO, VK_OBJECT_TYPE_BUFFER, "VB");
	SetObjectName(l_rhs, l_rhs->m_IBO, VK_OBJECT_TYPE_BUFFER, "IB");
#endif //  INNO_DEBUG

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeImpl(TextureComponent *rhs, void *textureData)
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

bool VKRenderingServer::InitializeImpl(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);

	bool l_result = true;

	l_result &= CreateOutputMergerTargets(l_rhs);

	l_result &= InitializeOutputMergerTargets(l_rhs);

	l_result &= ReserveFramebuffer(l_rhs);

	l_rhs->m_PipelineStateObject = AddPipelineStateObject();

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_result &= CreateRenderPass(l_rhs);
		l_result &= CreateViewportAndScissor(l_rhs);
		l_result &= CreateFramebuffers(l_rhs);
	}

	l_result &= CreateDescriptorSetLayout(m_dummyEmptyDescriptorLayout, l_rhs);

	l_result &= CreateDescriptorPool(l_rhs);

	l_result &= CreateDescriptorSets(l_rhs);

	l_result &= CreatePipelineLayout(l_rhs);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_result &= CreateGraphicsPipelines(l_rhs);
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_result &= CreateComputePipelines(l_rhs);
	}

	l_result &= CreateCommandPool(m_windowSurface, GPUEngineType::Graphics, l_rhs->m_GraphicsCommandPool);
	l_result &= CreateCommandPool(m_windowSurface, GPUEngineType::Compute, l_rhs->m_ComputeCommandPool);

	// Command lists are now created dynamically in CommandListBegin()
	// No pre-allocation needed in the new architecture

	l_result &= CreateCommandBuffers(l_rhs);

	l_rhs->m_Semaphores.resize(l_rhs->m_Framebuffers.size());

	for (size_t i = 0; i < l_rhs->m_Semaphores.size(); i++)
	{
		l_rhs->m_Semaphores[i] = AddSemaphore();
	}

	l_result &= CreateSyncPrimitives(l_rhs);
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool VKRenderingServer::InitializeImpl(ShaderProgramComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKShaderProgramComponent *>(rhs);

	bool l_result = true;

	l_rhs->m_vertexInputStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	l_rhs->m_vertexInputStateCInfo.vertexBindingDescriptionCount = 1;
	l_rhs->m_vertexInputStateCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexAttributeDescriptions.size());
	l_rhs->m_vertexInputStateCInfo.pVertexBindingDescriptions = &m_vertexBindingDescription;
	l_rhs->m_vertexInputStateCInfo.pVertexAttributeDescriptions = m_vertexAttributeDescriptions.data();

	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_VSHandle, l_rhs->m_ShaderFilePaths.m_VSPath);
		l_rhs->m_VSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_VSCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_rhs->m_VSCInfo.module = l_rhs->m_VSHandle;
		l_rhs->m_VSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_HSHandle, l_rhs->m_ShaderFilePaths.m_HSPath);
		l_rhs->m_HSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_HSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		l_rhs->m_HSCInfo.module = l_rhs->m_HSHandle;
		l_rhs->m_HSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_DSHandle, l_rhs->m_ShaderFilePaths.m_DSPath);
		l_rhs->m_DSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_DSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		l_rhs->m_DSCInfo.module = l_rhs->m_DSHandle;
		l_rhs->m_DSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_GSHandle, l_rhs->m_ShaderFilePaths.m_GSPath);
		l_rhs->m_GSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_GSCInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		l_rhs->m_GSCInfo.module = l_rhs->m_GSHandle;
		l_rhs->m_GSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_PSHandle, l_rhs->m_ShaderFilePaths.m_PSPath);
		l_rhs->m_PSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_PSCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_rhs->m_PSCInfo.module = l_rhs->m_PSHandle;
		l_rhs->m_PSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_CSHandle, l_rhs->m_ShaderFilePaths.m_CSPath);
		l_rhs->m_CSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_CSCInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		l_rhs->m_CSCInfo.module = l_rhs->m_CSHandle;
		l_rhs->m_CSCInfo.pName = "main";
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool VKRenderingServer::InitializeImpl(SamplerComponent *rhs)
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

bool VKRenderingServer::InitializeImpl(GPUBufferComponent *rhs)
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

	// @TODO: Fix it.
	//vkMapMemory(m_device, l_rhs->m_HostStagingMemory, 0, l_rhs->m_TotalSize, 0, &l_rhs->m_MappedMemory);

	l_rhs->m_GPUResourceType = GPUResourceType::Buffer;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

// @TODO: The command list should be passed as a parameter.
bool VKRenderingServer::UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferComponent*>(gpuBuffer);
	if (!l_rhs->m_DeviceLocalMemory)
		return true;
	
	// @TODO: Only copy the data that has been changed.
	CopyBuffer(m_globalCommandPool, m_graphicsQueue, l_rhs->m_HostStagingBuffer, l_rhs->m_DeviceLocalBuffer, l_rhs->m_TotalSize);

	return true;
}

bool VKRenderingServer::CreateImageView(VKTextureComponent *VKTextureComp)
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

bool VKRenderingServer::ReserveFramebuffer(VKRenderPassComponent* VKRenderPassComp)
{
	// @TODO: reconsider how to implement multi-frame support properly
	auto l_framebufferCount = GetSwapChainImageCount();
	VKRenderPassComp->m_Framebuffers.reserve(l_framebufferCount);
	for (size_t i = 0; i < l_framebufferCount; i++)
	{
		VKRenderPassComp->m_Framebuffers.emplace_back();
	}
	return true;
}

bool VKRenderingServer::CreateDescriptorSetLayoutBindings(VKRenderPassComponent *VKRenderPassComp)
{
	std::sort(VKRenderPassComp->m_ResourceBindingLayoutDescs.begin(), VKRenderPassComp->m_ResourceBindingLayoutDescs.end(), [&](ResourceBindingLayoutDesc A, ResourceBindingLayoutDesc B) {
		return A.m_DescriptorIndex < B.m_DescriptorIndex;
	});

	std::sort(VKRenderPassComp->m_ResourceBindingLayoutDescs.begin(), VKRenderPassComp->m_ResourceBindingLayoutDescs.end(), [&](ResourceBindingLayoutDesc A, ResourceBindingLayoutDesc B) {
		return A.m_DescriptorSetIndex < B.m_DescriptorSetIndex;
	});

	auto l_resourceBinderLayoutDescsSize = VKRenderPassComp->m_ResourceBindingLayoutDescs.size();

	size_t l_currentSetAbsoluteIndex = 0;
	size_t l_currentSetRelativeIndex = 0;

	for (size_t i = 0; i < l_resourceBinderLayoutDescsSize; i++)
	{
		auto l_resourceBinderLayoutDesc = VKRenderPassComp->m_ResourceBindingLayoutDescs[i];

		if (l_currentSetAbsoluteIndex != l_resourceBinderLayoutDesc.m_DescriptorSetIndex)
		{
			l_currentSetAbsoluteIndex = l_resourceBinderLayoutDesc.m_DescriptorSetIndex;
			l_currentSetRelativeIndex++;
		}
	}

	VKRenderPassComp->m_DescriptorSetLayoutBindings.reserve(l_resourceBinderLayoutDescsSize);
	VKRenderPassComp->m_DescriptorSetLayoutBindingIndices.resize(l_currentSetRelativeIndex + 1);

	l_currentSetAbsoluteIndex = 0;
	l_currentSetRelativeIndex = 0;
	size_t l_currentBindingOffset = 0;

	for (size_t i = 0; i < l_resourceBinderLayoutDescsSize; i++)
	{
		auto l_resourceBinderLayoutDesc = VKRenderPassComp->m_ResourceBindingLayoutDescs[i];

		VkDescriptorSetLayoutBinding l_descriptorLayoutBinding = {};
		l_descriptorLayoutBinding.binding = (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex;
		l_descriptorLayoutBinding.descriptorCount = 1;
		l_descriptorLayoutBinding.pImmutableSamplers = nullptr;
		l_descriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

		switch (l_resourceBinderLayoutDesc.m_GPUResourceType)
		{
		case GPUResourceType::Sampler:
			l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		case GPUResourceType::Image:
			if (l_resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
			{
				l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			}
			else
			{
				if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				{
					Log(Warning, "Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
				}
				else
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				}
			}
			break;
		case GPUResourceType::Buffer:
			if (l_resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
			{
				if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				}
				else
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				}
			}
			else
			{
				if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				{
					Log(Warning, "Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
				}
				else
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				}
			}
			break;
		default:
			break;
		}

		VKRenderPassComp->m_DescriptorSetLayoutBindings.emplace_back(l_descriptorLayoutBinding);

		if (l_currentSetAbsoluteIndex != l_resourceBinderLayoutDesc.m_DescriptorSetIndex)
		{
			l_currentSetAbsoluteIndex = l_resourceBinderLayoutDesc.m_DescriptorSetIndex;
			l_currentSetRelativeIndex++;
			VKRenderPassComp->m_DescriptorSetLayoutBindingIndices[l_currentSetRelativeIndex].m_LayoutBindingOffset = i;
		}

		VKRenderPassComp->m_DescriptorSetLayoutBindingIndices[l_currentSetRelativeIndex].m_SetIndex = l_currentSetAbsoluteIndex;
		VKRenderPassComp->m_DescriptorSetLayoutBindingIndices[l_currentSetRelativeIndex].m_BindingCount++;
	}

	return true;
}

bool VKRenderingServer::CreateDescriptorPool(VKRenderPassComponent *VKRenderPassComp)
{
	// Currently support less than 10 descriptor types actually
	std::array<uint32_t, 10> l_descriptorTypeCount = {};

	for (auto i : VKRenderPassComp->m_DescriptorSetLayoutBindings)
	{
		l_descriptorTypeCount[i.descriptorType]++;
	}

	// What a name
	auto l_VkDescriptorPoolSizesSize = std::count_if(l_descriptorTypeCount.begin(), l_descriptorTypeCount.end(), [](uint32_t i) { return i != 0; });

	std::vector<VkDescriptorPoolSize> l_descriptorPoolSizes(l_VkDescriptorPoolSizesSize);

	size_t l_index = 0;
	for (size_t i = 0; i < l_descriptorTypeCount.size(); i++)
	{
		if (l_descriptorTypeCount[i])
		{
			l_descriptorPoolSizes[l_index].type = VkDescriptorType(i);
			l_descriptorPoolSizes[l_index].descriptorCount = l_descriptorTypeCount[i];
			l_index++;
		}
	}

	auto l_result = CreateDescriptorPool(&l_descriptorPoolSizes[0], (uint32_t)l_descriptorPoolSizes.size(), (uint32_t)VKRenderPassComp->m_ResourceBindingLayoutDescs[VKRenderPassComp->m_ResourceBindingLayoutDescs.size() - 1].m_DescriptorSetIndex + 1, VKRenderPassComp->m_DescriptorPool);
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	if (l_result == VK_SUCCESS)
	{
		SetObjectName(VKRenderPassComp, VKRenderPassComp->m_DescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "DescriptorPool");
	}
#endif //  INNO_DEBUG
	return l_result;
}

bool VKRenderingServer::CreateDescriptorSetLayout(const VkDescriptorSetLayout& dummyEmptyDescriptorLayout, VKRenderPassComponent *VKRenderPassComp)
{
	bool l_result = true;
	if (VKRenderPassComp->m_ResourceBindingLayoutDescs.size())
	{
		l_result &= CreateDescriptorSetLayoutBindings(VKRenderPassComp);

		auto l_descriptorLayoutsSize = VKRenderPassComp->m_DescriptorSetLayoutBindingIndices.size();
		auto l_maximumSetIndex = VKRenderPassComp->m_DescriptorSetLayoutBindingIndices[l_descriptorLayoutsSize - 1].m_SetIndex;

		VKRenderPassComp->m_DescriptorSetLayouts.resize(l_maximumSetIndex + 1);
		for (size_t i = 0; i < VKRenderPassComp->m_DescriptorSetLayouts.size(); i++)
		{
			VKRenderPassComp->m_DescriptorSetLayouts[i] = dummyEmptyDescriptorLayout;
		}
		VKRenderPassComp->m_DescriptorSets.resize(l_maximumSetIndex + 1);

		for (size_t i = 0; i < l_descriptorLayoutsSize; i++)
		{
			auto l_descriptorSetLayoutBindingIndex = VKRenderPassComp->m_DescriptorSetLayoutBindingIndices[i];
			l_result &= CreateDescriptorSetLayout(&VKRenderPassComp->m_DescriptorSetLayoutBindings[l_descriptorSetLayoutBindingIndex.m_LayoutBindingOffset],
												  static_cast<uint32_t>(l_descriptorSetLayoutBindingIndex.m_BindingCount),
												  VKRenderPassComp->m_DescriptorSetLayouts[l_descriptorSetLayoutBindingIndex.m_SetIndex]);
		}
	}
	else
	{
		VKRenderPassComp->m_DescriptorSetLayouts.resize(1);
		VKRenderPassComp->m_DescriptorSetLayouts[0] = dummyEmptyDescriptorLayout;
		VKRenderPassComp->m_DescriptorSets.resize(1);
	}

	return true;
}

bool VKRenderingServer::CreateDescriptorSets(VKRenderPassComponent *VKRenderPassComp)
{
	bool l_result = true;
	for (size_t i = 0; i < VKRenderPassComp->m_DescriptorSetLayouts.size(); i++)
	{
		l_result &= CreateDescriptorSets(VKRenderPassComp->m_DescriptorPool, &VKRenderPassComp->m_DescriptorSetLayouts[i], VKRenderPassComp->m_DescriptorSets[i], 1);
	}

    return l_result;
}

bool VKRenderingServer::CreateRenderPass(VKRenderPassComponent *VKRenderPassComp, VkFormat* overrideFormat)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	l_PSO->m_RenderPassCInfo = {};
	l_PSO->m_RenderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	l_PSO->m_RenderPassCInfo.subpassCount = 1;

	l_PSO->m_SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	if (VKRenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
	{
		size_t colorAttachmentCount = VKRenderPassComp->m_RenderPassDesc.m_UseMultiFrames ? 1 : VKRenderPassComp->m_RenderPassDesc.m_RenderTargetCount;
		l_PSO->m_ColorAttachmentRefs.reserve(colorAttachmentCount);

		for (size_t i = 0; i < colorAttachmentCount; i++)
		{
			VkAttachmentReference l_colorAttachmentRef = {};
			l_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			l_colorAttachmentRef.attachment = (uint32_t)i;

			l_PSO->m_ColorAttachmentRefs.emplace_back(l_colorAttachmentRef);
		}

		VkAttachmentDescription l_colorAttachmentDesc = {};
		l_colorAttachmentDesc.format = overrideFormat ? *overrideFormat : GetTextureFormat(VKRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc);
		l_colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		l_colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		l_colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		l_colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		for (size_t i = 0; i < colorAttachmentCount; i++)
		{
			l_PSO->m_AttachmentDescs.emplace_back(l_colorAttachmentDesc);
		}

		// last attachment is depth attachment
		if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			l_PSO->m_DepthAttachmentRef.attachment = (uint32_t)colorAttachmentCount;
			l_PSO->m_DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription l_depthAttachmentDesc = {};

			if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable)
			{
				l_depthAttachmentDesc.format = VK_FORMAT_D24_UNORM_S8_UINT;
			}
			else
			{
				l_depthAttachmentDesc.format = VK_FORMAT_D32_SFLOAT;
			}
			l_depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			l_depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			l_depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			l_depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			l_depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			l_depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			l_PSO->m_AttachmentDescs.emplace_back(l_depthAttachmentDesc);
		}

		l_PSO->m_SubpassDesc.colorAttachmentCount = (uint32_t)colorAttachmentCount;
		l_PSO->m_RenderPassCInfo.attachmentCount = (uint32_t)l_PSO->m_AttachmentDescs.size();

		if (l_PSO->m_SubpassDesc.colorAttachmentCount)
		{
			l_PSO->m_SubpassDesc.pColorAttachments = &l_PSO->m_ColorAttachmentRefs[0];
		}

		if (l_PSO->m_DepthAttachmentRef.attachment)
		{
			l_PSO->m_SubpassDesc.pDepthStencilAttachment = &l_PSO->m_DepthAttachmentRef;
		}

		if (l_PSO->m_RenderPassCInfo.attachmentCount)
		{
			l_PSO->m_RenderPassCInfo.pAttachments = &l_PSO->m_AttachmentDescs[0];
		}
	}

	l_PSO->m_RenderPassCInfo.pSubpasses = &l_PSO->m_SubpassDesc;

	l_PSO->m_SubpassDeps.resize(4);

	l_PSO->m_SubpassDeps[0].srcSubpass = 0;
	l_PSO->m_SubpassDeps[0].dstSubpass = 0;
	l_PSO->m_SubpassDeps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	l_PSO->m_SubpassDeps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	l_PSO->m_SubpassDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	l_PSO->m_SubpassDeps[1].srcSubpass = 0;
	l_PSO->m_SubpassDeps[1].dstSubpass = 0;
	l_PSO->m_SubpassDeps[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	l_PSO->m_SubpassDeps[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	l_PSO->m_SubpassDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	l_PSO->m_SubpassDeps[2].srcSubpass = 0;
	l_PSO->m_SubpassDeps[2].dstSubpass = 0;
	l_PSO->m_SubpassDeps[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	l_PSO->m_SubpassDeps[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	l_PSO->m_SubpassDeps[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	
	l_PSO->m_SubpassDeps[3].srcSubpass = VK_SUBPASS_EXTERNAL;
	l_PSO->m_SubpassDeps[3].dstSubpass = 0;
	l_PSO->m_SubpassDeps[3].srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	l_PSO->m_SubpassDeps[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[3].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
 
	l_PSO->m_RenderPassCInfo.dependencyCount = l_PSO->m_SubpassDeps.size();
	l_PSO->m_RenderPassCInfo.pDependencies = &l_PSO->m_SubpassDeps[0];

	if (vkCreateRenderPass(m_device, &l_PSO->m_RenderPassCInfo, nullptr, &l_PSO->m_RenderPass) != VK_SUCCESS)
	{
		Log(Error, "", VKRenderPassComp->m_InstanceName.c_str(), " failed to create VkRenderPass!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_RenderPass, VK_OBJECT_TYPE_RENDER_PASS, "RenderPass");
#endif //  INNO_DEBUG

	Log(Verbose, "VkRenderPass has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKRenderingServer::CreateViewportAndScissor(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	l_PSO->m_Viewport.width = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width;
	l_PSO->m_Viewport.height = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height;
	l_PSO->m_Viewport.maxDepth = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MaxDepth;
	l_PSO->m_Viewport.minDepth = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MinDepth;
	l_PSO->m_Viewport.x = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_OriginX;
	l_PSO->m_Viewport.y = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_OriginY;

	l_PSO->m_Scissor.offset = {0, 0};
	l_PSO->m_Scissor.extent.width = (uint32_t)l_PSO->m_Viewport.width;
	l_PSO->m_Scissor.extent.height = (uint32_t)l_PSO->m_Viewport.height;

	return true;
}

bool VKRenderingServer::CreateFramebuffers(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	VkFramebufferCreateInfo l_framebufferCInfo = {};
	l_framebufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	l_framebufferCInfo.renderPass = l_PSO->m_RenderPass;
	l_framebufferCInfo.width = (uint32_t)l_PSO->m_Viewport.width;
	l_framebufferCInfo.height = (uint32_t)l_PSO->m_Viewport.height;
	l_framebufferCInfo.layers = 1;

	auto l_outputMergerTarget = VKRenderPassComp->m_OutputMergerTarget;
	auto l_attachmentCount = l_outputMergerTarget->m_ColorOutputs.size();

	// The depth-stencil attachment
	if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		l_attachmentCount += 1;

	auto l_swapChainImageCount = GetSwapChainImageCount();
	for (size_t i = 0; i < l_swapChainImageCount; i++)
	{
		std::vector<VkImageView> l_attachments(l_attachmentCount);

		if (VKRenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
		{
			for (size_t j = 0; j < l_outputMergerTarget->m_ColorOutputs.size(); j++)
			{
				auto l_VKTextureComp = reinterpret_cast<VKTextureComponent*>(l_outputMergerTarget->m_ColorOutputs[j]);

				// @TODO: Use the image view from textures of different frames
				l_attachments[j] = l_VKTextureComp->m_imageView;
			}
		}
		else
		{
			l_framebufferCInfo.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
		}

		if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			auto l_VKTextureComp = reinterpret_cast<VKTextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
			l_attachments[l_attachmentCount - 1] = l_VKTextureComp->m_imageView;
		}

		l_framebufferCInfo.attachmentCount = (uint32_t)l_attachments.size();
		l_framebufferCInfo.pAttachments = &l_attachments[0];
		if (vkCreateFramebuffer(m_device, &l_framebufferCInfo, nullptr, &VKRenderPassComp->m_Framebuffers[i]) != VK_SUCCESS)
		{
			Log(Error, "", VKRenderPassComp->m_InstanceName.c_str(), " failed to create VkFramebuffer!");
			continue;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		auto l_name = "FrameBuffer_" + std::to_string(i);
		SetObjectName(VKRenderPassComp, VKRenderPassComp->m_Framebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, l_name.c_str());
	#endif //  INNO_DEBUG
	}

	Log(Verbose, "VkFramebuffers have been created for ", VKRenderPassComp->m_InstanceName.c_str());

	return true;
}

bool VKRenderingServer::CreatePipelineLayout(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	l_PSO->m_PipelineLayoutCInfo = {};
	l_PSO->m_PipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	l_PSO->m_PipelineLayoutCInfo.setLayoutCount = static_cast<uint32_t>(VKRenderPassComp->m_DescriptorSetLayouts.size());
	l_PSO->m_PipelineLayoutCInfo.pSetLayouts = &VKRenderPassComp->m_DescriptorSetLayouts[0];

	if (VKRenderPassComp->m_PushConstantRanges.size() > 0)
	{
		l_PSO->m_PipelineLayoutCInfo.pushConstantRangeCount = static_cast<uint32_t>(VKRenderPassComp->m_PushConstantRanges.size());
		l_PSO->m_PipelineLayoutCInfo.pPushConstantRanges = VKRenderPassComp->m_PushConstantRanges.data();
	}

	if (vkCreatePipelineLayout(m_device, &l_PSO->m_PipelineLayoutCInfo, nullptr, &l_PSO->m_PipelineLayout) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkPipelineLayout!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_PipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "PipelineLayout");
#endif //  INNO_DEBUG

	Log(Verbose, "VkPipelineLayout has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKRenderingServer::CreateGraphicsPipelines(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);
	size_t colorAttachmentCount = VKRenderPassComp->m_RenderPassDesc.m_UseMultiFrames ? 1 : VKRenderPassComp->m_RenderPassDesc.m_RenderTargetCount;

	GenerateViewportState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);
	GenerateRasterizerState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
	GenerateDepthStencilState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
	GenerateBlendState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, colorAttachmentCount, l_PSO);

	// attach shader module and create pipeline
	auto l_VKSPC = reinterpret_cast<VKShaderProgramComponent *>(VKRenderPassComp->m_ShaderProgram);
	std::vector<VkPipelineShaderStageCreateInfo> l_shaderStageCInfos;
	l_shaderStageCInfos.reserve(6);

	if (l_VKSPC->m_ShaderFilePaths.m_VSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_VSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_HSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_HSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_DSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_DSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_GSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_GSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_PSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_PSCInfo);
	}

	l_PSO->m_GraphicsPipelineCInfo = {};
	l_PSO->m_GraphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	l_PSO->m_GraphicsPipelineCInfo.stageCount = (uint32_t)l_shaderStageCInfos.size();
	l_PSO->m_GraphicsPipelineCInfo.pStages = &l_shaderStageCInfos[0];
	l_PSO->m_GraphicsPipelineCInfo.pVertexInputState = &l_VKSPC->m_vertexInputStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pInputAssemblyState = &l_PSO->m_InputAssemblyStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pViewportState = &l_PSO->m_ViewportStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pRasterizationState = &l_PSO->m_RasterizationStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pMultisampleState = &l_PSO->m_MultisampleStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pDepthStencilState = &l_PSO->m_DepthStencilStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pColorBlendState = &l_PSO->m_ColorBlendStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.layout = l_PSO->m_PipelineLayout;
	l_PSO->m_GraphicsPipelineCInfo.renderPass = l_PSO->m_RenderPass;
	l_PSO->m_GraphicsPipelineCInfo.subpass = 0;
	l_PSO->m_GraphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &l_PSO->m_GraphicsPipelineCInfo, nullptr, &l_PSO->m_Pipeline) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkPipeline for GraphicsPipeline!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_Pipeline, VK_OBJECT_TYPE_PIPELINE, "GraphicsPipeline");
#endif //  INNO_DEBUG

	Log(Verbose, "VkPipeline for GraphicsPipeline has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKRenderingServer::CreateComputePipelines(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	// attach shader module and create pipeline
	auto l_VKSPC = reinterpret_cast<VKShaderProgramComponent *>(VKRenderPassComp->m_ShaderProgram);

	l_PSO->m_ComputePipelineCInfo = {};
	l_PSO->m_ComputePipelineCInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	l_PSO->m_ComputePipelineCInfo.stage = l_VKSPC->m_CSCInfo;
	l_PSO->m_ComputePipelineCInfo.layout = l_PSO->m_PipelineLayout;
	l_PSO->m_ComputePipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &l_PSO->m_ComputePipelineCInfo, nullptr, &l_PSO->m_Pipeline) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkPipeline for ComputePipeline!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_Pipeline, VK_OBJECT_TYPE_PIPELINE, "ComputePipeline");
#endif //  INNO_DEBUG

	Log(Verbose, "VkPipeline for ComputePipeline has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKRenderingServer::CreateCommandBuffers(VKRenderPassComponent *VKRenderPassComp)
{
	// In the new architecture, command buffers are allocated dynamically when CommandListBegin is called
	// This function now only needs to ensure the command pools are ready
	// The actual command buffer allocation happens in AddCommandList() and CommandListBegin()
	
	// Command pools should already be created in CreateCommandPools()
	if (VKRenderPassComp->m_GraphicsCommandPool == VK_NULL_HANDLE)
	{
		Log(Error, "Graphics command pool not created for render pass ", VKRenderPassComp->m_InstanceName.c_str());
		return false;
	}
	
	if (VKRenderPassComp->m_ComputeCommandPool == VK_NULL_HANDLE) 
	{
		Log(Error, "Compute command pool not created for render pass ", VKRenderPassComp->m_InstanceName.c_str());
		return false;
	}
	
	Log(Verbose, "Command pools are ready for dynamic command buffer allocation for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKRenderingServer::CreateSyncPrimitives(VKRenderPassComponent *VKRenderPassComp)
{
	VkSemaphoreTypeCreateInfo l_timelineCreateInfo = {};
	l_timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	l_timelineCreateInfo.pNext = NULL;
	l_timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	l_timelineCreateInfo.initialValue = 0;

	VkSemaphoreCreateInfo l_semaphoreInfo;
	l_semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	l_semaphoreInfo.pNext = &l_timelineCreateInfo;
	l_semaphoreInfo.flags = 0;

	VKRenderPassComp->m_SubmitInfo = {};

	for (size_t i = 0; i < VKRenderPassComp->m_Semaphores.size(); i++)
	{
		auto l_VKSemaphore = reinterpret_cast<VKSemaphore *>(VKRenderPassComp->m_Semaphores[i]);

		if (vkCreateSemaphore(m_device, &l_semaphoreInfo, nullptr, &l_VKSemaphore->m_GraphicsSemaphore) != VK_SUCCESS)
		{
			Log(Error, "Failed to create Graphics semaphore!");
			return false;
		}
		
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	auto l_graphicsName = "GraphicsSemaphore_" + std::to_string(i);
	SetObjectName(VKRenderPassComp, l_VKSemaphore->m_GraphicsSemaphore, VK_OBJECT_TYPE_SEMAPHORE, l_graphicsName.c_str());
#endif //  INNO_DEBUG

		if (vkCreateSemaphore(m_device, &l_semaphoreInfo, nullptr, &l_VKSemaphore->m_ComputeSemaphore) != VK_SUCCESS)
		{
			Log(Error, "Failed to create Compute semaphore!");
			return false;
		}
		
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	auto l_computeName = "ComputeSemaphore_" + std::to_string(i);
	SetObjectName(VKRenderPassComp, l_VKSemaphore->m_ComputeSemaphore, VK_OBJECT_TYPE_SEMAPHORE, l_computeName.c_str());
#endif //  INNO_DEBUG
	}

	Log(Verbose, "Synchronization primitives has been created for ", VKRenderPassComp->m_InstanceName.c_str());

	return true;
}

bool VKRenderingServer::GenerateViewportState(ViewportDesc viewportDesc, VKPipelineStateObject *PSO)
{
	PSO->m_ViewportStateCInfo = {};
	PSO->m_ViewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	PSO->m_ViewportStateCInfo.viewportCount = 1;
	PSO->m_ViewportStateCInfo.pViewports = &PSO->m_Viewport;
	PSO->m_ViewportStateCInfo.scissorCount = 1;
	PSO->m_ViewportStateCInfo.pScissors = &PSO->m_Scissor;

	return true;
}

bool VKRenderingServer::GenerateRasterizerState(RasterizerDesc rasterizerDesc, VKPipelineStateObject *PSO)
{
	PSO->m_InputAssemblyStateCInfo = {};
	PSO->m_InputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	switch (rasterizerDesc.m_PrimitiveTopology)
	{
	case PrimitiveTopology::Point:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		break;
	case PrimitiveTopology::Line:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		break;
	case PrimitiveTopology::TriangleList:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	case PrimitiveTopology::TriangleStrip:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		break;
	case PrimitiveTopology::Patch:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		break;
	default:
		break;
	}
	PSO->m_InputAssemblyStateCInfo.primitiveRestartEnable = false;

	PSO->m_RasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	if (rasterizerDesc.m_UseCulling)
	{
		switch (rasterizerDesc.m_RasterizerCullMode)
		{
		case RasterizerCullMode::Back:
			PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		case RasterizerCullMode::Front:
			PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		default:
			break;
		}
	}
	else
	{
		PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_NONE;
	}

	switch (rasterizerDesc.m_RasterizerFillMode)
	{
	case Type::RasterizerFillMode::Point:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_POINT;
		break;
	case Type::RasterizerFillMode::Wireframe:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_LINE;
		break;
	case Type::RasterizerFillMode::Solid:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
		break;
	default:
		break;
	}

	PSO->m_RasterizationStateCInfo.frontFace = (rasterizerDesc.m_RasterizerFaceWinding == RasterizerFaceWinding::CCW) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
	PSO->m_RasterizationStateCInfo.lineWidth = 1.0f;

	PSO->m_MultisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	PSO->m_MultisampleStateCInfo.sampleShadingEnable = rasterizerDesc.m_AllowMultisample;
	PSO->m_MultisampleStateCInfo.rasterizationSamples = rasterizerDesc.m_AllowMultisample ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;

	return true;
}

bool VKRenderingServer::GenerateDepthStencilState(DepthStencilDesc depthStencilDesc, VKPipelineStateObject *PSO)
{	
	PSO->m_DepthStencilStateCInfo = {};
	PSO->m_DepthStencilStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	PSO->m_DepthStencilStateCInfo.depthTestEnable = depthStencilDesc.m_DepthEnable;
	PSO->m_DepthStencilStateCInfo.depthWriteEnable = depthStencilDesc.m_AllowDepthWrite;
	PSO->m_DepthStencilStateCInfo.depthCompareOp = GetComparisionFunctionEnum(depthStencilDesc.m_DepthComparisionFunction);
	PSO->m_DepthStencilStateCInfo.depthBoundsTestEnable = VK_FALSE;
	PSO->m_DepthStencilStateCInfo.minDepthBounds = 0.0f; // Optional
	PSO->m_DepthStencilStateCInfo.maxDepthBounds = 1.0f; // Optional

	PSO->m_DepthStencilStateCInfo.stencilTestEnable = depthStencilDesc.m_StencilEnable;

	PSO->m_DepthStencilStateCInfo.front.failOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilFailOperation);
	PSO->m_DepthStencilStateCInfo.front.passOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilPassOperation);
	PSO->m_DepthStencilStateCInfo.front.depthFailOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilStateCInfo.front.compareOp = GetComparisionFunctionEnum(depthStencilDesc.m_FrontFaceStencilComparisionFunction);
	PSO->m_DepthStencilStateCInfo.front.compareMask = 0xFF;
	PSO->m_DepthStencilStateCInfo.front.writeMask = depthStencilDesc.m_StencilWriteMask;
	PSO->m_DepthStencilStateCInfo.front.reference = depthStencilDesc.m_StencilReference;

	PSO->m_DepthStencilStateCInfo.back.failOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilFailOperation);
	PSO->m_DepthStencilStateCInfo.back.passOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilPassOperation);
	PSO->m_DepthStencilStateCInfo.back.depthFailOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilStateCInfo.back.compareOp = GetComparisionFunctionEnum(depthStencilDesc.m_BackFaceStencilComparisionFunction);
	PSO->m_DepthStencilStateCInfo.back.compareMask = 0xFF;
	PSO->m_DepthStencilStateCInfo.back.writeMask = depthStencilDesc.m_StencilWriteMask;
	PSO->m_DepthStencilStateCInfo.back.reference = depthStencilDesc.m_StencilReference;

	return true;
}

bool VKRenderingServer::GenerateBlendState(BlendDesc blendDesc, size_t colorBlendAttachmentCount, VKPipelineStateObject *PSO)
{
	PSO->m_ColorBlendStateCInfo = {};
	PSO->m_ColorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineColorBlendAttachmentState l_colorBlendAttachmentState = {};
	l_colorBlendAttachmentState.blendEnable = blendDesc.m_UseBlend;
	l_colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_colorBlendAttachmentState.srcColorBlendFactor = GetBlendFactorEnum(blendDesc.m_SourceRGBFactor);
	l_colorBlendAttachmentState.srcAlphaBlendFactor = GetBlendFactorEnum(blendDesc.m_SourceAlphaFactor);
	l_colorBlendAttachmentState.dstColorBlendFactor = GetBlendFactorEnum(blendDesc.m_DestinationRGBFactor);
	l_colorBlendAttachmentState.dstAlphaBlendFactor = GetBlendFactorEnum(blendDesc.m_DestinationAlphaFactor);
	l_colorBlendAttachmentState.colorBlendOp = GetBlendOperation(blendDesc.m_BlendOperation);
	l_colorBlendAttachmentState.alphaBlendOp = GetBlendOperation(blendDesc.m_BlendOperation);

	PSO->m_ColorBlendAttachmentStates.reserve(colorBlendAttachmentCount);

	for (size_t i = 0; i < colorBlendAttachmentCount; i++)
	{
		PSO->m_ColorBlendAttachmentStates.emplace_back(l_colorBlendAttachmentState);
	}

	PSO->m_ColorBlendStateCInfo.logicOpEnable = VK_FALSE;
	PSO->m_ColorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;

	PSO->m_ColorBlendStateCInfo.blendConstants[0] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[1] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[2] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[3] = 0.0f;

	if (PSO->m_ColorBlendAttachmentStates.size())
	{
		PSO->m_ColorBlendStateCInfo.attachmentCount = (uint32_t)PSO->m_ColorBlendAttachmentStates.size();
		PSO->m_ColorBlendStateCInfo.pAttachments = &PSO->m_ColorBlendAttachmentStates[0];
	}

	return true;
}
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

bool VKGraphicsService::InitializeImpl(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);

	bool l_result = true;

	l_result &= g_Engine->Get<GraphicsResourceService>()->CreateOutputMergerTargets(l_rhs);

	l_result &= g_Engine->Get<GraphicsResourceService>()->InitializeOutputMergerTargets(l_rhs);

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

bool VKGraphicsService::CreateDescriptorSetLayoutBindings(VKRenderPassComponent *VKRenderPassComp)
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

bool VKGraphicsService::CreateDescriptorPool(VKRenderPassComponent *VKRenderPassComp)
{
	// Currently support less than 10 descriptor types actually
	std::array<uint32_t, 10> l_descriptorTypeCount = {};

	for (auto i : VKRenderPassComp->m_DescriptorSetLayoutBindings)
	{
		l_descriptorTypeCount[i.descriptorType]++;
	}

	// What a name
	auto l_VkDescriptorPoolSizesSize = std::count_if(l_descriptorTypeCount.begin(), l_descriptorTypeCount.end(), [](uint32_t i) { return i != 0; });

	Inno::Array<VkDescriptorPoolSize> l_descriptorPoolSizes(l_VkDescriptorPoolSizesSize);

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

bool VKGraphicsService::CreateDescriptorSetLayout(const VkDescriptorSetLayout& dummyEmptyDescriptorLayout, VKRenderPassComponent *VKRenderPassComp)
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

bool VKGraphicsService::CreateDescriptorSets(VKRenderPassComponent *VKRenderPassComp)
{
	bool l_result = true;
	for (size_t i = 0; i < VKRenderPassComp->m_DescriptorSetLayouts.size(); i++)
	{
		l_result &= CreateDescriptorSets(VKRenderPassComp->m_DescriptorPool, &VKRenderPassComp->m_DescriptorSetLayouts[i], VKRenderPassComp->m_DescriptorSets[i], 1);
	}

    return l_result;
}

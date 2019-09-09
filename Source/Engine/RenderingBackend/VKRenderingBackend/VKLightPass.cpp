#include "VKLightPass.h"
#include "VKRenderingBackendUtilities.h"
#include "../../Component/VKRenderingBackendComponent.h"

#include "VKOpaquePass.h"

using namespace VKRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace VKLightPass
{
	EntityID m_EntityID;

	VKRenderPassComponent* m_VKRPC;

	VKShaderProgramComponent* m_VKSPC;

	ShaderFilePaths m_shaderFilePaths = { "VK//lightPass.vert.spv/", "", "", "", "VK//lightPass.frag.spv/" };
}

bool VKLightPass::initialize()
{
	m_EntityID = InnoMath::createEntityID();

	// add shader component
	m_VKSPC = addVKShaderProgramComponent(m_EntityID);

	initializeVKShaderProgramComponent(m_VKSPC, m_shaderFilePaths);

	// add render pass component
	m_VKRPC = addVKRenderPassComponent();

	m_VKRPC->m_renderPassDesc = VKRenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_VKRPC->m_renderPassDesc.RTNumber = 1;
	m_VKRPC->m_renderPassDesc.useMultipleFramebuffers = false;

	// create descriptor pool
	VkDescriptorPoolSize l_cameraUBODescriptorPoolSize = {};
	VkDescriptorPoolSize l_sunUBODescriptorPoolSize = {};
	VkDescriptorPoolSize l_pointLightUBODescriptorPoolSize = {};
	VkDescriptorPoolSize l_sphereLightUBODescriptorPoolSize = {};
	VkDescriptorPoolSize l_opaquePassRT0DescriptorPoolSize = {};
	VkDescriptorPoolSize l_opaquePassRT1DescriptorPoolSize = {};
	VkDescriptorPoolSize l_opaquePassRT2DescriptorPoolSize = {};

	l_cameraUBODescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	l_cameraUBODescriptorPoolSize.descriptorCount = 1;

	l_sunUBODescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	l_sunUBODescriptorPoolSize.descriptorCount = 1;

	l_pointLightUBODescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	l_pointLightUBODescriptorPoolSize.descriptorCount = 1;

	l_sphereLightUBODescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	l_sphereLightUBODescriptorPoolSize.descriptorCount = 1;

	l_opaquePassRT0DescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	l_opaquePassRT0DescriptorPoolSize.descriptorCount = 1;

	l_opaquePassRT1DescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	l_opaquePassRT1DescriptorPoolSize.descriptorCount = 1;

	l_opaquePassRT2DescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	l_opaquePassRT2DescriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize l_UBODescriptorPoolSizes[] = {
		l_cameraUBODescriptorPoolSize ,
		l_sunUBODescriptorPoolSize,
		l_pointLightUBODescriptorPoolSize,
		l_sphereLightUBODescriptorPoolSize,
		l_opaquePassRT0DescriptorPoolSize,
		l_opaquePassRT1DescriptorPoolSize,
		l_opaquePassRT2DescriptorPoolSize
	};

	createDescriptorPool(l_UBODescriptorPoolSizes, 7, 1, m_VKRPC->m_descriptorPool);

	// sub-pass
	VkAttachmentReference l_colorAttachmentRef = {};

	l_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	for (size_t i = 0; i < m_VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		l_colorAttachmentRef.attachment = (uint32_t)i;
		m_VKRPC->colorAttachmentRefs.emplace_back(l_colorAttachmentRef);
	}

	// render pass
	VkAttachmentDescription l_colorAttachmentDesc = {};

	l_colorAttachmentDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	l_colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	l_colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	l_colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	l_colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	l_colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	l_colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	l_colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	for (size_t i = 0; i < m_VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		m_VKRPC->attachmentDescs.emplace_back(l_colorAttachmentDesc);
	}

	m_VKRPC->renderPassCInfo.subpassCount = 1;

	// set descriptor set layout binding info
	VkDescriptorSetLayoutBinding cameraUBODescriptorLayoutBinding = {};
	cameraUBODescriptorLayoutBinding.binding = 0;
	cameraUBODescriptorLayoutBinding.descriptorCount = 1;
	cameraUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	cameraUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(cameraUBODescriptorLayoutBinding);

	VkDescriptorSetLayoutBinding sunUBODescriptorLayoutBinding = {};
	sunUBODescriptorLayoutBinding.binding = 1;
	sunUBODescriptorLayoutBinding.descriptorCount = 1;
	sunUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sunUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	sunUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(sunUBODescriptorLayoutBinding);

	VkDescriptorSetLayoutBinding pointLightUBODescriptorLayoutBinding = {};
	pointLightUBODescriptorLayoutBinding.binding = 2;
	pointLightUBODescriptorLayoutBinding.descriptorCount = 1;
	pointLightUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pointLightUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	pointLightUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(pointLightUBODescriptorLayoutBinding);

	VkDescriptorSetLayoutBinding sphereLightUBODescriptorLayoutBinding = {};
	sphereLightUBODescriptorLayoutBinding.binding = 3;
	sphereLightUBODescriptorLayoutBinding.descriptorCount = 1;
	sphereLightUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sphereLightUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	sphereLightUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(sphereLightUBODescriptorLayoutBinding);

	VkDescriptorSetLayoutBinding opaquePassRT0LayoutBinding = {};
	opaquePassRT0LayoutBinding.binding = 4;
	opaquePassRT0LayoutBinding.descriptorCount = 1;
	opaquePassRT0LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	opaquePassRT0LayoutBinding.pImmutableSamplers = nullptr;
	opaquePassRT0LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(opaquePassRT0LayoutBinding);

	VkDescriptorSetLayoutBinding opaquePassRT1LayoutBinding = {};
	opaquePassRT1LayoutBinding.binding = 5;
	opaquePassRT1LayoutBinding.descriptorCount = 1;
	opaquePassRT1LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	opaquePassRT1LayoutBinding.pImmutableSamplers = nullptr;
	opaquePassRT1LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(opaquePassRT1LayoutBinding);

	VkDescriptorSetLayoutBinding opaquePassRT2LayoutBinding = {};
	opaquePassRT2LayoutBinding.binding = 6;
	opaquePassRT2LayoutBinding.descriptorCount = 1;
	opaquePassRT2LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	opaquePassRT2LayoutBinding.pImmutableSamplers = nullptr;
	opaquePassRT2LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(opaquePassRT2LayoutBinding);

	// set descriptor buffer info
	VkDescriptorBufferInfo cameraUBODescriptorBufferInfo = {};
	cameraUBODescriptorBufferInfo.buffer = VKRenderingBackendComponent::get().m_cameraUBO;
	cameraUBODescriptorBufferInfo.offset = 0;
	cameraUBODescriptorBufferInfo.range = sizeof(CameraGPUData);

	VkWriteDescriptorSet cameraUBOWriteDescriptorSet = {};
	cameraUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cameraUBOWriteDescriptorSet.dstBinding = 0;
	cameraUBOWriteDescriptorSet.dstArrayElement = 0;
	cameraUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraUBOWriteDescriptorSet.descriptorCount = 1;
	cameraUBOWriteDescriptorSet.pBufferInfo = &cameraUBODescriptorBufferInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(cameraUBOWriteDescriptorSet);

	VkDescriptorBufferInfo sunUBODescriptorBufferInfo = {};
	sunUBODescriptorBufferInfo.buffer = VKRenderingBackendComponent::get().m_sunUBO;
	sunUBODescriptorBufferInfo.offset = 0;
	sunUBODescriptorBufferInfo.range = sizeof(SunGPUData);

	VkWriteDescriptorSet sunUBOWriteDescriptorSet = {};
	sunUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sunUBOWriteDescriptorSet.dstBinding = 1;
	sunUBOWriteDescriptorSet.dstArrayElement = 0;
	sunUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sunUBOWriteDescriptorSet.descriptorCount = 1;
	sunUBOWriteDescriptorSet.pBufferInfo = &sunUBODescriptorBufferInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(sunUBOWriteDescriptorSet);

	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	VkDescriptorBufferInfo pointLightUBODescriptorBufferInfo = {};
	pointLightUBODescriptorBufferInfo.buffer = VKRenderingBackendComponent::get().m_pointLightUBO;
	pointLightUBODescriptorBufferInfo.offset = 0;
	pointLightUBODescriptorBufferInfo.range = sizeof(PointLightGPUData) * l_renderingCapability.maxPointLights;

	VkWriteDescriptorSet pointLightUBOWriteDescriptorSet = {};
	pointLightUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	pointLightUBOWriteDescriptorSet.dstBinding = 2;
	pointLightUBOWriteDescriptorSet.dstArrayElement = 0;
	pointLightUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pointLightUBOWriteDescriptorSet.descriptorCount = 1;
	pointLightUBOWriteDescriptorSet.pBufferInfo = &pointLightUBODescriptorBufferInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(pointLightUBOWriteDescriptorSet);

	VkDescriptorBufferInfo sphereLightUBODescriptorBufferInfo = {};
	sphereLightUBODescriptorBufferInfo.buffer = VKRenderingBackendComponent::get().m_sphereLightUBO;
	sphereLightUBODescriptorBufferInfo.offset = 0;
	sphereLightUBODescriptorBufferInfo.range = sizeof(SphereLightGPUData) * l_renderingCapability.maxSphereLights;

	VkWriteDescriptorSet sphereLightUBOWriteDescriptorSet = {};
	sphereLightUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sphereLightUBOWriteDescriptorSet.dstBinding = 3;
	sphereLightUBOWriteDescriptorSet.dstArrayElement = 0;
	sphereLightUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sphereLightUBOWriteDescriptorSet.descriptorCount = 1;
	sphereLightUBOWriteDescriptorSet.pBufferInfo = &sphereLightUBODescriptorBufferInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(sphereLightUBOWriteDescriptorSet);

	VkDescriptorImageInfo opaquePassRT0ImageInfo;
	opaquePassRT0ImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	opaquePassRT0ImageInfo.imageView = VKOpaquePass::getVKRPC()->m_VKTDCs[0]->m_imageView;
	opaquePassRT0ImageInfo.sampler = VKRenderingBackendComponent::get().m_deferredRTSampler;

	VkWriteDescriptorSet opaquePassRT0WriteDescriptorSet = {};
	opaquePassRT0WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	opaquePassRT0WriteDescriptorSet.dstBinding = 4;
	opaquePassRT0WriteDescriptorSet.dstArrayElement = 0;
	opaquePassRT0WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	opaquePassRT0WriteDescriptorSet.descriptorCount = 1;
	opaquePassRT0WriteDescriptorSet.pImageInfo = &opaquePassRT0ImageInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(opaquePassRT0WriteDescriptorSet);

	VkDescriptorImageInfo opaquePassRT1ImageInfo;
	opaquePassRT1ImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	opaquePassRT1ImageInfo.imageView = VKOpaquePass::getVKRPC()->m_VKTDCs[1]->m_imageView;
	opaquePassRT1ImageInfo.sampler = VKRenderingBackendComponent::get().m_deferredRTSampler;

	VkWriteDescriptorSet opaquePassRT1WriteDescriptorSet = {};
	opaquePassRT1WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	opaquePassRT1WriteDescriptorSet.dstBinding = 5;
	opaquePassRT1WriteDescriptorSet.dstArrayElement = 0;
	opaquePassRT1WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	opaquePassRT1WriteDescriptorSet.descriptorCount = 1;
	opaquePassRT1WriteDescriptorSet.pImageInfo = &opaquePassRT1ImageInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(opaquePassRT1WriteDescriptorSet);

	VkDescriptorImageInfo opaquePassRT2ImageInfo;
	opaquePassRT2ImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	opaquePassRT2ImageInfo.imageView = VKOpaquePass::getVKRPC()->m_VKTDCs[2]->m_imageView;
	opaquePassRT2ImageInfo.sampler = VKRenderingBackendComponent::get().m_deferredRTSampler;

	VkWriteDescriptorSet opaquePassRT2WriteDescriptorSet = {};
	opaquePassRT2WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	opaquePassRT2WriteDescriptorSet.dstBinding = 6;
	opaquePassRT2WriteDescriptorSet.dstArrayElement = 0;
	opaquePassRT2WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	opaquePassRT2WriteDescriptorSet.descriptorCount = 1;
	opaquePassRT2WriteDescriptorSet.pImageInfo = &opaquePassRT2ImageInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(opaquePassRT2WriteDescriptorSet);

	// set pipeline fix stages info
	m_VKRPC->inputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	m_VKRPC->inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;

	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	VkExtent2D l_extent = {};
	l_extent.width = l_screenResolution.x;
	l_extent.height = l_screenResolution.y;

	m_VKRPC->viewport.x = 0.0f;
	m_VKRPC->viewport.y = 0.0f;
	m_VKRPC->viewport.width = (float)l_extent.width;
	m_VKRPC->viewport.height = (float)l_extent.height;
	m_VKRPC->viewport.minDepth = 0.0f;
	m_VKRPC->viewport.maxDepth = 1.0f;

	m_VKRPC->scissor.offset = { 0, 0 };
	m_VKRPC->scissor.extent = l_extent;

	m_VKRPC->viewportStateCInfo.viewportCount = 1;
	m_VKRPC->viewportStateCInfo.scissorCount = 1;

	m_VKRPC->rasterizationStateCInfo.depthClampEnable = VK_FALSE;
	m_VKRPC->rasterizationStateCInfo.rasterizerDiscardEnable = VK_FALSE;
	m_VKRPC->rasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	m_VKRPC->rasterizationStateCInfo.lineWidth = 1.0f;
	m_VKRPC->rasterizationStateCInfo.cullMode = VK_CULL_MODE_NONE;
	m_VKRPC->rasterizationStateCInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_VKRPC->rasterizationStateCInfo.depthBiasEnable = VK_FALSE;

	m_VKRPC->multisampleStateCInfo.sampleShadingEnable = VK_FALSE;
	m_VKRPC->multisampleStateCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState l_colorBlendAttachmentState = {};
	l_colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_colorBlendAttachmentState.blendEnable = VK_FALSE;
	for (size_t i = 0; i < m_VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		m_VKRPC->colorBlendAttachmentStates.emplace_back(l_colorBlendAttachmentState);
	}

	m_VKRPC->colorBlendStateCInfo.logicOpEnable = VK_FALSE;
	m_VKRPC->colorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;
	m_VKRPC->colorBlendStateCInfo.blendConstants[0] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[1] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[2] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[3] = 0.0f;

	m_VKRPC->descriptorSetLayouts.resize(1);
	initializeVKRenderPassComponent(m_VKRPC, m_VKSPC);

	m_VKRPC->descriptorSets.resize(1);
	createDescriptorSets(m_VKRPC->m_descriptorPool, m_VKRPC->descriptorSetLayouts[0], m_VKRPC->descriptorSets[0], 1);

	m_VKRPC->writeDescriptorSets[0].dstSet = m_VKRPC->descriptorSets[0];
	m_VKRPC->writeDescriptorSets[1].dstSet = m_VKRPC->descriptorSets[0];
	m_VKRPC->writeDescriptorSets[2].dstSet = m_VKRPC->descriptorSets[0];
	m_VKRPC->writeDescriptorSets[3].dstSet = m_VKRPC->descriptorSets[0];
	m_VKRPC->writeDescriptorSets[4].dstSet = m_VKRPC->descriptorSets[0];
	m_VKRPC->writeDescriptorSets[5].dstSet = m_VKRPC->descriptorSets[0];
	m_VKRPC->writeDescriptorSets[6].dstSet = m_VKRPC->descriptorSets[0];

	updateDescriptorSet(m_VKRPC->writeDescriptorSets.data(), static_cast<uint32_t>(m_VKRPC->writeDescriptorSets.size()));

	return true;
}

bool VKLightPass::update()
{
	waitForFence(m_VKRPC);

	recordCommand(m_VKRPC, 0, [&]() {
		vkCmdBindDescriptorSets(m_VKRPC->m_commandBuffers[0],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_VKRPC->m_pipelineLayout,
			0,
			1,
			&m_VKRPC->descriptorSets[0], 0, nullptr);
		auto l_MDC = getVKMeshDataComponent(MeshShapeType::QUAD);
		recordDrawCall(m_VKRPC, 0, l_MDC);
	});

	return true;
}

bool VKLightPass::render()
{
	VkSemaphore l_availableSemaphores[] = {
		VKOpaquePass::getVKRPC()->m_renderFinishedSemaphores[0],
	};

	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};

	m_VKRPC->submitInfo.waitSemaphoreCount = 1;
	m_VKRPC->submitInfo.pWaitSemaphores = l_availableSemaphores;
	m_VKRPC->submitInfo.pWaitDstStageMask = waitStages;

	submitCommand(m_VKRPC, 0);

	return true;
}

bool VKLightPass::terminate()
{
	destroyVKShaderProgramComponent(m_VKSPC);
	destroyVKRenderPassComponent(m_VKRPC);

	return true;
}

VKRenderPassComponent * VKLightPass::getVKRPC()
{
	return m_VKRPC;
}
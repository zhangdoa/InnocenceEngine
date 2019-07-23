#include "VKFinalBlendPass.h"
#include "VKRenderingBackendUtilities.h"
#include "../../Component/VKRenderingBackendComponent.h"

#include "VKLightPass.h"

using namespace VKRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace VKFinalBlendPass
{
	EntityID m_entityID;

	VKRenderPassComponent* m_VKRPC;

	VKShaderProgramComponent* m_VKSPC;

	ShaderFilePaths m_shaderFilePaths = { "VK//finalBlendPass.vert.spv/", "", "", "", "VK//finalBlendPass.frag.spv/" };

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
}

bool VKFinalBlendPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	// add shader component
	m_VKSPC = addVKShaderProgramComponent(m_entityID);

	initializeVKShaderProgramComponent(m_VKSPC, m_shaderFilePaths);

	// add render pass component
	m_VKRPC = addVKRenderPassComponent();

	m_VKRPC->m_renderPassDesc = VKRenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_VKRPC->m_renderPassDesc.RTNumber = (unsigned int)VKRenderingBackendComponent::get().m_swapChainImages.size();
	m_VKRPC->m_renderPassDesc.useMultipleFramebuffers = (m_VKRPC->m_renderPassDesc.RTNumber > 1);

	VkTextureDataDesc l_VkTextureDataDesc;
	l_VkTextureDataDesc.imageType = VK_IMAGE_TYPE_2D;
	l_VkTextureDataDesc.samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	l_VkTextureDataDesc.magFilterParam = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	l_VkTextureDataDesc.minFilterParam = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	l_VkTextureDataDesc.format = VKRenderingBackendComponent::get().m_windowSurfaceFormat.format;
	l_VkTextureDataDesc.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

	// initialize manually
	bool l_result = true;

	l_result &= reserveRenderTargets(m_VKRPC);

	// use device created swap chain VkImages
	for (size_t i = 0; i < VKRenderingBackendComponent::get().m_swapChainImages.size(); i++)
	{
		m_VKRPC->m_VKTDCs[i]->m_VkTextureDataDesc = l_VkTextureDataDesc;
		m_VKRPC->m_VKTDCs[i]->m_image = VKRenderingBackendComponent::get().m_swapChainImages[i];
		createImageView(m_VKRPC->m_VKTDCs[i]);
	}

	// create descriptor pool
	VkDescriptorPoolSize l_RTSamplerDescriptorPoolSize = {};
	l_RTSamplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
	l_RTSamplerDescriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize l_RTTextureDescriptorPoolSize = {};
	l_RTTextureDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_RTTextureDescriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize l_descriptorPoolSize[] = { l_RTSamplerDescriptorPoolSize , l_RTTextureDescriptorPoolSize };

	l_result &= createDescriptorPool(l_descriptorPoolSize, 2, 1, m_VKRPC->m_descriptorPool);

	// sub-pass
	VkAttachmentReference l_attachmentRef = {};
	l_attachmentRef.attachment = 0;
	l_attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_VKRPC->colorAttachmentRefs.emplace_back(l_attachmentRef);

	// render pass
	VkAttachmentDescription attachmentDesc = {};
	attachmentDesc.format = VKRenderingBackendComponent::get().m_windowSurfaceFormat.format;
	attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	m_VKRPC->attachmentDescs.emplace_back(attachmentDesc);

	m_VKRPC->renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_VKRPC->renderPassCInfo.subpassCount = 1;

	// set descriptor set layout binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = &VKRenderingBackendComponent::get().m_deferredRTSampler;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(samplerLayoutBinding);

	VkDescriptorSetLayoutBinding textureLayoutBinding = {};
	textureLayoutBinding.binding = 1;
	textureLayoutBinding.descriptorCount = 1;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	textureLayoutBinding.pImmutableSamplers = nullptr;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(textureLayoutBinding);

	// set descriptor image info
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageInfo.imageView = VKLightPass::getVKRPC()->m_VKTDCs[0]->m_imageView;

	VkWriteDescriptorSet basePassRTWriteDescriptorSet = {};
	basePassRTWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	basePassRTWriteDescriptorSet.dstBinding = 1;
	basePassRTWriteDescriptorSet.dstArrayElement = 0;
	basePassRTWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	basePassRTWriteDescriptorSet.descriptorCount = 1;
	basePassRTWriteDescriptorSet.pImageInfo = &imageInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(basePassRTWriteDescriptorSet);

	// set pipeline fix stages info
	m_VKRPC->inputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_VKRPC->inputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	m_VKRPC->inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;

	m_VKRPC->viewport.x = 0.0f;
	m_VKRPC->viewport.y = 0.0f;
	m_VKRPC->viewport.width = (float)VKRenderingBackendComponent::get().m_windowSurfaceExtent.width;
	m_VKRPC->viewport.height = (float)VKRenderingBackendComponent::get().m_windowSurfaceExtent.height;
	m_VKRPC->viewport.minDepth = 0.0f;
	m_VKRPC->viewport.maxDepth = 1.0f;

	m_VKRPC->scissor.offset = { 0, 0 };
	m_VKRPC->scissor.extent = VKRenderingBackendComponent::get().m_windowSurfaceExtent;

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
	m_VKRPC->colorBlendAttachmentStates.emplace_back(l_colorBlendAttachmentState);

	m_VKRPC->colorBlendStateCInfo.logicOpEnable = VK_FALSE;
	m_VKRPC->colorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;
	m_VKRPC->colorBlendStateCInfo.blendConstants[0] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[1] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[2] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[3] = 0.0f;

	l_result &= createRenderPass(m_VKRPC);

	if (m_VKRPC->m_renderPassDesc.useMultipleFramebuffers)
	{
		l_result &= createMultipleFramebuffers(m_VKRPC);
	}
	else
	{
		l_result &= createSingleFramebuffer(m_VKRPC);
	}

	m_VKRPC->descriptorSetLayouts.resize(1);
	l_result &= createDescriptorSetLayout(m_VKRPC->descriptorSetLayoutBindings.data(), static_cast<uint32_t>(m_VKRPC->descriptorSetLayoutBindings.size()), m_VKRPC->descriptorSetLayouts[0]);

	l_result &= createPipelineLayout(m_VKRPC);

	l_result &= createGraphicsPipelines(m_VKRPC, m_VKSPC);

	m_VKRPC->descriptorSets.resize(1);
	l_result &= createDescriptorSets(m_VKRPC->m_descriptorPool, m_VKRPC->descriptorSetLayouts[0], m_VKRPC->descriptorSets[0], 1);

	m_VKRPC->writeDescriptorSets[0].dstSet = m_VKRPC->descriptorSets[0];

	l_result &= updateDescriptorSet(m_VKRPC->writeDescriptorSets.data(), static_cast<uint32_t>(m_VKRPC->writeDescriptorSets.size()));

	l_result = createCommandBuffers(m_VKRPC);

	m_VKRPC->m_maxFramesInFlight = 2;
	m_imageAvailableSemaphores.resize(m_VKRPC->m_maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < m_VKRPC->m_maxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(
			VKRenderingBackendComponent::get().m_device,
			&semaphoreInfo,
			nullptr,
			&m_imageAvailableSemaphores[i])
			!= VK_SUCCESS)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create swap chain image available semaphores!");
			return false;
		}
	}

	l_result = createSyncPrimitives(m_VKRPC);

	return true;
}

bool VKFinalBlendPass::update()
{
	waitForFence(m_VKRPC);

	for (size_t i = 0; i < m_VKRPC->m_commandBuffers.size(); i++)
	{
		recordCommand(m_VKRPC, (unsigned int)i, [&]() {
			vkCmdBindDescriptorSets(m_VKRPC->m_commandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_VKRPC->m_pipelineLayout,
				0,
				1,
				&m_VKRPC->descriptorSets[0], 0, nullptr);
			auto l_Mesh = getVKMeshDataComponent(MeshShapeType::QUAD);
			recordDrawCall(m_VKRPC, (unsigned int)i, l_Mesh);
		});
	}

	return true;
}

bool VKFinalBlendPass::render()
{
	// acquire an image from swap chain
	thread_local uint32_t imageIndex;
	vkAcquireNextImageKHR(
		VKRenderingBackendComponent::get().m_device,
		VKRenderingBackendComponent::get().m_swapChain,
		std::numeric_limits<uint64_t>::max(),
		m_imageAvailableSemaphores[m_VKRPC->m_currentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

	// set swap chain image available wait semaphore
	VkSemaphore l_availableSemaphores[] = {
		VKLightPass::getVKRPC()->m_renderFinishedSemaphores[0],
	m_imageAvailableSemaphores[m_VKRPC->m_currentFrame]
	};

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	m_VKRPC->submitInfo.waitSemaphoreCount = 2;
	m_VKRPC->submitInfo.pWaitSemaphores = l_availableSemaphores;
	m_VKRPC->submitInfo.pWaitDstStageMask = waitStages;

	submitCommand(m_VKRPC, imageIndex);

	// present the swap chain image to the front screen
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	// wait semaphore
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_VKRPC->m_renderFinishedSemaphores[m_VKRPC->m_currentFrame];

	// swap chain
	VkSwapchainKHR swapChains[] = { VKRenderingBackendComponent::get().m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(VKRenderingBackendComponent::get().m_presentQueue, &presentInfo);

	m_VKRPC->m_currentFrame = (m_VKRPC->m_currentFrame + 1) % m_VKRPC->m_maxFramesInFlight;

	return true;
}

bool VKFinalBlendPass::terminate()
{
	destroyVKShaderProgramComponent(m_VKSPC);
	for (size_t i = 0; i < m_VKRPC->m_maxFramesInFlight; i++)
	{
		vkDestroySemaphore(VKRenderingBackendComponent::get().m_device, m_imageAvailableSemaphores[i], nullptr);
	}
	destroyVKRenderPassComponent(m_VKRPC);

	return true;
}

VKRenderPassComponent * VKFinalBlendPass::getVKRPC()
{
	return m_VKRPC;
}
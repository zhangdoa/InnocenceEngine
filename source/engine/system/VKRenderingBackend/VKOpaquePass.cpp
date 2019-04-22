#include "VKOpaquePass.h"
#include "VKRenderingSystemUtilities.h"
#include "../../component/VKRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace VKRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKOpaquePass
{
	EntityID m_entityID;

	VKRenderPassComponent* m_VKRPC;

	VKShaderProgramComponent* m_VKSPC;

	ShaderFilePaths m_shaderFilePaths = { "VK//opaquePass.vert.spv" , "", "VK//opaquePass.frag.spv" };
}

bool VKOpaquePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	// add shader component
	m_VKSPC = addVKShaderProgramComponent(m_entityID);

	initializeVKShaderProgramComponent(m_VKSPC, m_shaderFilePaths);

	// add render pass component
	m_VKRPC = addVKRenderPassComponent();

	m_VKRPC->m_renderPassDesc = VKRenderingSystemComponent::get().m_deferredRenderPassDesc;
	m_VKRPC->m_renderPassDesc.RTNumber = 4;
	m_VKRPC->m_renderPassDesc.useMultipleFramebuffers = false;

	// sub-pass
	VkAttachmentReference l_attachmentRef = {};

	l_attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	for (size_t i = 0; i < m_VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		l_attachmentRef.attachment = (uint32_t)i;
		m_VKRPC->attachmentRefs.emplace_back(l_attachmentRef);
	}

	// render pass
	VkAttachmentDescription l_attachmentDesc = {};

	l_attachmentDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	l_attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	l_attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	l_attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	l_attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	l_attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	l_attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	l_attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	for (size_t i = 0; i < m_VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		m_VKRPC->attachmentDescs.emplace_back(l_attachmentDesc);
	}

	m_VKRPC->renderPassCInfo.subpassCount = 1;

	// set descriptor set layout binding info
	VkDescriptorSetLayoutBinding cameraUBODescriptorLayoutBinding = {};
	cameraUBODescriptorLayoutBinding.binding = 0;
	cameraUBODescriptorLayoutBinding.descriptorCount = 1;
	cameraUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	cameraUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(cameraUBODescriptorLayoutBinding);

	VkDescriptorSetLayoutBinding meshUBODescriptorLayoutBinding = {};
	meshUBODescriptorLayoutBinding.binding = 1;
	meshUBODescriptorLayoutBinding.descriptorCount = 1;
	meshUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	meshUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	meshUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(meshUBODescriptorLayoutBinding);

	// set descriptor buffer info
	VkDescriptorBufferInfo cameraUBODescriptorBufferInfo = {};
	cameraUBODescriptorBufferInfo.buffer = VKRenderingSystemComponent::get().m_cameraUBO;
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

	VkDescriptorBufferInfo meshUBODescriptorBufferInfo = {};
	meshUBODescriptorBufferInfo.buffer = VKRenderingSystemComponent::get().m_meshUBO;
	meshUBODescriptorBufferInfo.offset = 0;
	meshUBODescriptorBufferInfo.range = sizeof(MeshGPUData);

	VkWriteDescriptorSet meshUBOWriteDescriptorSet = {};
	meshUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	meshUBOWriteDescriptorSet.dstBinding = 1;
	meshUBOWriteDescriptorSet.dstArrayElement = 0;
	meshUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	meshUBOWriteDescriptorSet.descriptorCount = 1;
	meshUBOWriteDescriptorSet.pBufferInfo = &meshUBODescriptorBufferInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(meshUBOWriteDescriptorSet);

	// set pipeline fix stages info
	m_VKRPC->inputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	m_VKRPC->inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

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
	m_VKRPC->rasterizationStateCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	m_VKRPC->rasterizationStateCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

	initializeVKRenderPassComponent(m_VKRPC, m_VKSPC);

	createDescriptorSets(VKRenderingSystemComponent::get().m_UBODescriptorPool, m_VKRPC->descriptorSetLayout, m_VKRPC->descriptorSet, 1);

	m_VKRPC->writeDescriptorSets[0].dstSet = m_VKRPC->descriptorSet;

	m_VKRPC->writeDescriptorSets[1].dstSet = m_VKRPC->descriptorSet;

	updateDescriptorSet(m_VKRPC);

	return true;
}

bool VKOpaquePass::update()
{
	waitForFence(m_VKRPC);

	unsigned int l_sizeofMeshGPUData = sizeof(MeshGPUData);

	recordCommand(m_VKRPC, 0, [&]() {
		unsigned int offsetCount = 0;

		while (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.size() > 0)
		{
			GeometryPassGPUData l_geometryPassGPUData = {};

			auto l_dynamicOffset = l_sizeofMeshGPUData * offsetCount;

			if (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.tryPop(l_geometryPassGPUData))
			{
				vkCmdBindDescriptorSets(m_VKRPC->m_commandBuffers[0],
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_VKRPC->m_pipelineLayout,
					0,
					1,
					&m_VKRPC->descriptorSet, 1, &l_dynamicOffset);

				recordDrawCall(m_VKRPC, 0, reinterpret_cast<VKMeshDataComponent*>(l_geometryPassGPUData.MDC));

				offsetCount++;
			}
		};
	});

	return true;
}

bool VKOpaquePass::render()
{
	summitCommand(m_VKRPC, 0);

	return true;
}

bool VKOpaquePass::terminate()
{
	destroyVKShaderProgramComponent(m_VKSPC);
	destroyVKRenderPassComponent(m_VKRPC);

	return true;
}

VKRenderPassComponent * VKOpaquePass::getVKRPC()
{
	return m_VKRPC;
}
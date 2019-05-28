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

	ShaderFilePaths m_shaderFilePaths = { "VK//opaquePass.vert.spv/", "", "", "", "VK//opaquePass.frag.spv/" };
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
	m_VKRPC->m_renderPassDesc.useDepthAttachment = true;

	// create descriptor pool
	VkDescriptorPoolSize l_cameraUBODescriptorPoolSize = {};
	VkDescriptorPoolSize l_meshUBODescriptorPoolSize = {};
	VkDescriptorPoolSize l_materialUBODescriptorPoolSize = {};

	l_cameraUBODescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	l_cameraUBODescriptorPoolSize.descriptorCount = 1;

	l_meshUBODescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	l_meshUBODescriptorPoolSize.descriptorCount = 1;

	l_materialUBODescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	l_materialUBODescriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize l_UBODescriptorPoolSizes[] = { l_cameraUBODescriptorPoolSize , l_meshUBODescriptorPoolSize, l_materialUBODescriptorPoolSize };
	createDescriptorPool(l_UBODescriptorPoolSizes, 3, 1, m_VKRPC->m_descriptorPool);

	// sub-pass
	VkAttachmentReference l_colorAttachmentRef = {};

	l_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	for (size_t i = 0; i < m_VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		l_colorAttachmentRef.attachment = (uint32_t)i;
		m_VKRPC->colorAttachmentRefs.emplace_back(l_colorAttachmentRef);
	}

	// last attachment is depth attachment
	m_VKRPC->depthAttachmentRef.attachment = m_VKRPC->m_renderPassDesc.RTNumber;
	m_VKRPC->depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

	VkAttachmentDescription l_depthAttachmentDesc = {};

	l_depthAttachmentDesc.format = VK_FORMAT_D32_SFLOAT;
	l_depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	l_depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	l_depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	l_depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	l_depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	l_depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	l_depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	m_VKRPC->attachmentDescs.emplace_back(l_depthAttachmentDesc);

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

	VkDescriptorSetLayoutBinding materialUBODescriptorLayoutBinding = {};
	materialUBODescriptorLayoutBinding.binding = 2;
	materialUBODescriptorLayoutBinding.descriptorCount = 1;
	materialUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	materialUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	materialUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(materialUBODescriptorLayoutBinding);

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

	VkDescriptorBufferInfo materialUBODescriptorBufferInfo = {};
	materialUBODescriptorBufferInfo.buffer = VKRenderingSystemComponent::get().m_materialUBO;
	materialUBODescriptorBufferInfo.offset = 0;
	materialUBODescriptorBufferInfo.range = sizeof(MaterialGPUData);

	VkWriteDescriptorSet materialUBOWriteDescriptorSet = {};
	materialUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	materialUBOWriteDescriptorSet.dstBinding = 2;
	materialUBOWriteDescriptorSet.dstArrayElement = 0;
	materialUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	materialUBOWriteDescriptorSet.descriptorCount = 1;
	materialUBOWriteDescriptorSet.pBufferInfo = &materialUBODescriptorBufferInfo;
	m_VKRPC->writeDescriptorSets.emplace_back(materialUBOWriteDescriptorSet);

	// set pipeline fix stages info
	m_VKRPC->inputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_VKRPC->inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilCInfo = {};
	depthStencilCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCInfo.depthTestEnable = VK_TRUE;
	depthStencilCInfo.depthWriteEnable = VK_TRUE;
	depthStencilCInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCInfo.minDepthBounds = 0.0f; // Optional
	depthStencilCInfo.maxDepthBounds = 1.0f; // Optional
	depthStencilCInfo.stencilTestEnable = VK_FALSE;
	depthStencilCInfo.front = {}; // Optional
	depthStencilCInfo.back = {}; // Optional
	m_VKRPC->pipelineCInfo.pDepthStencilState = &depthStencilCInfo;

	auto l_screenResolution = g_pCoreSystem->getRenderingFrontendSystem()->getScreenResolution();

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

	createDescriptorSets(m_VKRPC->m_descriptorPool, m_VKRPC->descriptorSetLayout, m_VKRPC->descriptorSet, 1);

	m_VKRPC->writeDescriptorSets[0].dstSet = m_VKRPC->descriptorSet;
	m_VKRPC->writeDescriptorSets[1].dstSet = m_VKRPC->descriptorSet;
	m_VKRPC->writeDescriptorSets[2].dstSet = m_VKRPC->descriptorSet;

	updateDescriptorSet(m_VKRPC);

	return true;
}

bool VKOpaquePass::update()
{
	waitForFence(m_VKRPC);

	unsigned int l_sizeofMeshGPUData = sizeof(MeshGPUData);
	unsigned int l_sizeofMaterialGPUData = sizeof(MaterialGPUData);

	recordCommand(m_VKRPC, 0, [&]() {
		unsigned int offsetCount = 0;

		for (unsigned int i = 0; i < RenderingFrontendSystemComponent::get().m_opaquePassDrawcallCount; i++)
		{
			auto l_meshUBOOffset = l_sizeofMeshGPUData * offsetCount;
			auto l_materialUBOOffset = l_sizeofMaterialGPUData * offsetCount;
			unsigned int l_dynamicOffsets[] = { l_meshUBOOffset, l_materialUBOOffset };

			auto l_opaquePassGPUData = RenderingFrontendSystemComponent::get().m_opaquePassGPUDatas[i];

			vkCmdBindDescriptorSets(m_VKRPC->m_commandBuffers[0],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_VKRPC->m_pipelineLayout,
				0,
				1,
				&m_VKRPC->descriptorSet, 2, l_dynamicOffsets);

			recordDrawCall(m_VKRPC, 0, reinterpret_cast<VKMeshDataComponent*>(l_opaquePassGPUData.MDC));

			offsetCount++;
		}
	});

	return true;
}

bool VKOpaquePass::render()
{
	submitCommand(m_VKRPC, 0);

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
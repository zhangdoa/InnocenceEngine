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
	m_VKRPC->m_renderPassDesc.RTNumber = 1;
	m_VKRPC->m_renderPassDesc.useMultipleFramebuffers = false;

	// sub-pass
	m_VKRPC->attachmentRef.attachment = 0;
	m_VKRPC->attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_VKRPC->subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	m_VKRPC->subpassDesc.colorAttachmentCount = 1;
	m_VKRPC->subpassDesc.pColorAttachments = &m_VKRPC->attachmentRef;

	// render pass
	VkAttachmentDescription attachmentDesc1 = {};
	VkAttachmentDescription attachmentDesc2;
	VkAttachmentDescription attachmentDesc3;
	VkAttachmentDescription attachmentDesc4;

	attachmentDesc1.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	attachmentDesc1.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc1.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc1.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc1.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc1.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc1.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc1.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachmentDesc2 = attachmentDesc1;
	attachmentDesc3 = attachmentDesc1;
	attachmentDesc4 = attachmentDesc1;

	m_VKRPC->attachmentDescs.emplace_back(attachmentDesc1);
	m_VKRPC->attachmentDescs.emplace_back(attachmentDesc2);
	m_VKRPC->attachmentDescs.emplace_back(attachmentDesc3);
	m_VKRPC->attachmentDescs.emplace_back(attachmentDesc4);

	m_VKRPC->renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_VKRPC->renderPassCInfo.attachmentCount = 1;
	m_VKRPC->renderPassCInfo.pAttachments = &m_VKRPC->attachmentDescs[0];
	m_VKRPC->renderPassCInfo.subpassCount = 1;
	m_VKRPC->renderPassCInfo.pSubpasses = &m_VKRPC->subpassDesc;

	// set descriptor pool size info
	VkDescriptorPoolSize cameraUBODescPoolSize;
	cameraUBODescPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraUBODescPoolSize.descriptorCount = 1;
	m_VKRPC->descriptorPoolSizes.emplace_back(cameraUBODescPoolSize);

	// set descriptor set layout binding info
	VkDescriptorSetLayoutBinding cameraUBODescriptorLayoutBinding = {};
	cameraUBODescriptorLayoutBinding.binding = 0;
	cameraUBODescriptorLayoutBinding.descriptorCount = 1;
	cameraUBODescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraUBODescriptorLayoutBinding.pImmutableSamplers = nullptr;
	cameraUBODescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_VKRPC->descriptorSetLayoutBindings.emplace_back(cameraUBODescriptorLayoutBinding);

	// set descriptor buffer info
	VkDescriptorBufferInfo cameraUBODescriptorBufferInfo = {};
	cameraUBODescriptorBufferInfo.buffer = VKRenderingSystemComponent::get().m_cameraUBO;
	cameraUBODescriptorBufferInfo.offset = 0;
	cameraUBODescriptorBufferInfo.range = sizeof(CameraGPUData);
	m_VKRPC->descriptorBufferInfos.emplace_back(cameraUBODescriptorBufferInfo);

	VkWriteDescriptorSet cameraUBOWriteDescriptorSet = {};
	cameraUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cameraUBOWriteDescriptorSet.dstBinding = 0;
	cameraUBOWriteDescriptorSet.dstArrayElement = 0;
	cameraUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraUBOWriteDescriptorSet.descriptorCount = 1;
	cameraUBOWriteDescriptorSet.pBufferInfo = &m_VKRPC->descriptorBufferInfos[0];
	m_VKRPC->writeDescriptorSets.emplace_back(cameraUBOWriteDescriptorSet);

	// set push constant info
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(MeshGPUData);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	m_VKRPC->pushConstantRanges.emplace_back(pushConstantRange);

	// set pipeline fix stages info
	m_VKRPC->inputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_VKRPC->inputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	m_VKRPC->inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;

	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	VkExtent2D l_extent;
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

	m_VKRPC->viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_VKRPC->viewportStateCInfo.viewportCount = 1;
	m_VKRPC->viewportStateCInfo.pViewports = &m_VKRPC->viewport;
	m_VKRPC->viewportStateCInfo.scissorCount = 1;
	m_VKRPC->viewportStateCInfo.pScissors = &m_VKRPC->scissor;

	m_VKRPC->rasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_VKRPC->rasterizationStateCInfo.depthClampEnable = VK_FALSE;
	m_VKRPC->rasterizationStateCInfo.rasterizerDiscardEnable = VK_FALSE;
	m_VKRPC->rasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	m_VKRPC->rasterizationStateCInfo.lineWidth = 1.0f;
	m_VKRPC->rasterizationStateCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	m_VKRPC->rasterizationStateCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	m_VKRPC->rasterizationStateCInfo.depthBiasEnable = VK_FALSE;

	m_VKRPC->multisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_VKRPC->multisampleStateCInfo.sampleShadingEnable = VK_FALSE;
	m_VKRPC->multisampleStateCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	m_VKRPC->colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_VKRPC->colorBlendAttachmentState.blendEnable = VK_FALSE;

	m_VKRPC->colorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_VKRPC->colorBlendStateCInfo.logicOpEnable = VK_FALSE;
	m_VKRPC->colorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;
	m_VKRPC->colorBlendStateCInfo.attachmentCount = 1;
	m_VKRPC->colorBlendStateCInfo.pAttachments = &m_VKRPC->colorBlendAttachmentState;
	m_VKRPC->colorBlendStateCInfo.blendConstants[0] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[1] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[2] = 0.0f;
	m_VKRPC->colorBlendStateCInfo.blendConstants[3] = 0.0f;

	m_VKRPC->pipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_VKRPC->pipelineLayoutCInfo.setLayoutCount = 1;

	initializeVKRenderPassComponent(m_VKRPC, m_VKSPC);
	return true;
}

bool VKOpaquePass::update()
{
	waitForFence(m_VKRPC);

	recordCommand(m_VKRPC, 0, [&]() {
		recordDescriptorBinding(m_VKRPC, 0);

		while (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.size() > 0)
		{
			GeometryPassGPUData l_geometryPassGPUData = {};

			if (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.tryPop(l_geometryPassGPUData))
			{
				vkCmdPushConstants(
					m_VKRPC->m_commandBuffers[0],
					m_VKRPC->m_pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0,
					sizeof(MeshGPUData),
					&l_geometryPassGPUData.meshGPUData);
				recordDrawCall(m_VKRPC, 0, reinterpret_cast<VKMeshDataComponent*>(l_geometryPassGPUData.MDC));
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
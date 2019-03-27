#pragma once
#include "VKRenderingSystemUtilities.h"
#include "../../component/VKRenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	bool createShaderModule(VkShaderModule& vkShaderModule, const std::string& shaderFilePath);

	void* m_VKRenderPassComponentPool;
	void* m_VKShaderProgramComponentPool;
}

bool VKRenderingSystemNS::initializeComponentPool()
{
	m_VKRenderPassComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKRenderPassComponent), 32);
	m_VKShaderProgramComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKShaderProgramComponent), 128);

	return true;
}

VKRenderPassComponent* VKRenderingSystemNS::addVKRenderPassComponent(unsigned int RTNum, TextureDataDesc RTDesc, VKShaderProgramComponent* VKSPC)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_VKRenderPassComponentPool, sizeof(VKRenderPassComponent));
	auto l_VKRPC = new(l_rawPtr)VKRenderPassComponent();

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VKRenderingSystemComponent::get().m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(VKRenderingSystemComponent::get().m_device, &renderPassInfo, nullptr, &l_VKRPC->m_renderPass) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create render pass!");
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: render pass has been created.");

	l_VKRPC->m_viewport.x = 0.0f;
	l_VKRPC->m_viewport.y = 0.0f;
	l_VKRPC->m_viewport.width = (float)VKRenderingSystemComponent::get().m_swapChainExtent.width;
	l_VKRPC->m_viewport.height = (float)VKRenderingSystemComponent::get().m_swapChainExtent.height;
	l_VKRPC->m_viewport.minDepth = 0.0f;
	l_VKRPC->m_viewport.maxDepth = 1.0f;

	l_VKRPC->m_scissor.offset = { 0, 0 };
	l_VKRPC->m_scissor.extent = VKRenderingSystemComponent::get().m_swapChainExtent;

	l_VKRPC->m_viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	l_VKRPC->m_viewportStateCInfo.viewportCount = 1;
	l_VKRPC->m_viewportStateCInfo.pViewports = &l_VKRPC->m_viewport;
	l_VKRPC->m_viewportStateCInfo.scissorCount = 1;
	l_VKRPC->m_viewportStateCInfo.pScissors = &l_VKRPC->m_scissor;

	l_VKRPC->m_rasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	l_VKRPC->m_rasterizationStateCInfo.depthClampEnable = VK_FALSE;
	l_VKRPC->m_rasterizationStateCInfo.rasterizerDiscardEnable = VK_FALSE;
	l_VKRPC->m_rasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	l_VKRPC->m_rasterizationStateCInfo.lineWidth = 1.0f;
	l_VKRPC->m_rasterizationStateCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	l_VKRPC->m_rasterizationStateCInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	l_VKRPC->m_rasterizationStateCInfo.depthBiasEnable = VK_FALSE;

	l_VKRPC->m_multisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	l_VKRPC->m_multisampleStateCInfo.sampleShadingEnable = VK_FALSE;
	l_VKRPC->m_multisampleStateCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	l_VKRPC->m_colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_VKRPC->m_colorBlendAttachmentState.blendEnable = VK_FALSE;

	l_VKRPC->m_colorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	l_VKRPC->m_colorBlendStateCInfo.logicOpEnable = VK_FALSE;
	l_VKRPC->m_colorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;
	l_VKRPC->m_colorBlendStateCInfo.attachmentCount = 1;
	l_VKRPC->m_colorBlendStateCInfo.pAttachments = &l_VKRPC->m_colorBlendAttachmentState;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[0] = 0.0f;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[1] = 0.0f;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[2] = 0.0f;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[3] = 0.0f;

	l_VKRPC->m_pipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	l_VKRPC->m_pipelineLayoutCInfo.setLayoutCount = 0;
	l_VKRPC->m_pipelineLayoutCInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(VKRenderingSystemComponent::get().m_device, &l_VKRPC->m_pipelineLayoutCInfo, nullptr, &l_VKRPC->m_pipelineLayout) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create pipeline layout!");
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: pipeline layout has been created.");

	std::vector<VkPipelineShaderStageCreateInfo> l_shaderStages = { VKSPC->m_vertexShaderStageCInfo, VKSPC->m_fragmentShaderStageCInfo };

	l_VKRPC->m_pipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	l_VKRPC->m_pipelineCInfo.stageCount = 2;
	l_VKRPC->m_pipelineCInfo.pStages = &l_shaderStages[0];
	l_VKRPC->m_pipelineCInfo.pVertexInputState = &VKSPC->m_vertexInputStateCInfo;
	l_VKRPC->m_pipelineCInfo.pInputAssemblyState = &VKSPC->m_inputAssemblyStateCInfo;
	l_VKRPC->m_pipelineCInfo.pViewportState = &l_VKRPC->m_viewportStateCInfo;
	l_VKRPC->m_pipelineCInfo.pRasterizationState = &l_VKRPC->m_rasterizationStateCInfo;
	l_VKRPC->m_pipelineCInfo.pMultisampleState = &l_VKRPC->m_multisampleStateCInfo;
	l_VKRPC->m_pipelineCInfo.pColorBlendState = &l_VKRPC->m_colorBlendStateCInfo;
	l_VKRPC->m_pipelineCInfo.layout = l_VKRPC->m_pipelineLayout;
	l_VKRPC->m_pipelineCInfo.renderPass = l_VKRPC->m_renderPass;
	l_VKRPC->m_pipelineCInfo.subpass = 0;
	l_VKRPC->m_pipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(VKRenderingSystemComponent::get().m_device, VK_NULL_HANDLE, 1, &l_VKRPC->m_pipelineCInfo, nullptr, &l_VKRPC->m_pipeline) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to to create graphics pipeline!");
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: graphics pipeline has been created.");

	return l_VKRPC;
}

bool VKRenderingSystemNS::destroyVKRenderPassComponent(VKRenderPassComponent* VKRPC)
{
	vkDestroyPipeline(VKRenderingSystemComponent::get().m_device, VKRPC->m_pipeline, nullptr);
	vkDestroyPipelineLayout(VKRenderingSystemComponent::get().m_device, VKRPC->m_pipelineLayout, nullptr);
	vkDestroyRenderPass(VKRenderingSystemComponent::get().m_device, VKRPC->m_renderPass, nullptr);

	return true;
}

VKMeshDataComponent * VKRenderingSystemNS::generateVKMeshDataComponent(MeshDataComponent * rhs)
{
	return nullptr;
}

VKTextureDataComponent * VKRenderingSystemNS::generateVKTextureDataComponent(TextureDataComponent * rhs)
{
	return nullptr;
}

VKMeshDataComponent * VKRenderingSystemNS::addVKMeshDataComponent(EntityID rhs)
{
	return nullptr;
}

VKTextureDataComponent * VKRenderingSystemNS::addVKTextureDataComponent(EntityID rhs)
{
	return nullptr;
}

VKMeshDataComponent * VKRenderingSystemNS::getVKMeshDataComponent(EntityID rhs)
{
	return nullptr;
}

VKTextureDataComponent * VKRenderingSystemNS::getVKTextureDataComponent(EntityID rhs)
{
	return nullptr;
}

void VKRenderingSystemNS::drawMesh(EntityID rhs)
{
}

void VKRenderingSystemNS::drawMesh(MeshDataComponent * MDC)
{
}

void VKRenderingSystemNS::drawMesh(size_t indicesSize, VKMeshDataComponent * VKMDC)
{
}

bool VKRenderingSystemNS::createShaderModule(VkShaderModule& vkShaderModule, const std::string& shaderFilePath)
{
	auto l_binData = g_pCoreSystem->getFileSystem()->loadBinaryFile(shaderFilePath);

	VkShaderModuleCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	l_createInfo.codeSize = l_binData.size();
	l_createInfo.pCode = reinterpret_cast<const uint32_t*>(l_binData.data());

	if (vkCreateShaderModule(VKRenderingSystemComponent::get().m_device, &l_createInfo, nullptr, &vkShaderModule) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create shader module for: " + shaderFilePath + "!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: innoShader: " + shaderFilePath + " has been loaded.");
	return true;
}

VKShaderProgramComponent * VKRenderingSystemNS::addVKShaderProgramComponent(const EntityID & rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_VKShaderProgramComponentPool, sizeof(VKShaderProgramComponent));
	auto l_VKSPC = new(l_rawPtr)VKShaderProgramComponent();
	return l_VKSPC;
}

bool VKRenderingSystemNS::initializeVKShaderProgramComponent(VKShaderProgramComponent * rhs, const ShaderFilePaths & shaderFilePaths)
{
	bool l_result = true;
	if (shaderFilePaths.m_VSPath != "")
	{
		l_result &= createShaderModule(rhs->m_vertexShaderModule, shaderFilePaths.m_VSPath);
		rhs->m_vertexShaderStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rhs->m_vertexShaderStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		rhs->m_vertexShaderStageCInfo.module = rhs->m_vertexShaderModule;
		rhs->m_vertexShaderStageCInfo.pName = "main";

		rhs->m_vertexInputStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		rhs->m_vertexInputStateCInfo.vertexBindingDescriptionCount = 0;
		rhs->m_vertexInputStateCInfo.vertexAttributeDescriptionCount = 0;

		rhs->m_inputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		rhs->m_inputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		rhs->m_inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;
	}
	if (shaderFilePaths.m_FSPath != "")
	{
		l_result &= createShaderModule(rhs->m_fragmentShaderModule, shaderFilePaths.m_FSPath);
		rhs->m_fragmentShaderStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rhs->m_fragmentShaderStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		rhs->m_fragmentShaderStageCInfo.module = rhs->m_fragmentShaderModule;
		rhs->m_fragmentShaderStageCInfo.pName = "main";
	}

	return l_result;
}

bool VKRenderingSystemNS::activateVKShaderProgramComponent(VKShaderProgramComponent * rhs)
{
	return false;
}
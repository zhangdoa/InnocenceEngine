#pragma once
#include "ShaderProgramComponent.h"
#include "vulkan/vulkan.h"

class VKShaderProgramComponent : public ShaderProgramComponent
{
public:
	VkShaderModule m_VSHandle;
	VkShaderModule m_HSHandle;
	VkShaderModule m_DSHandle;
	VkShaderModule m_GSHandle;
	VkShaderModule m_PSHandle;
	VkShaderModule m_CSHandle;
	VkPipelineVertexInputStateCreateInfo m_vertexInputStateCInfo = {};
	VkPipelineShaderStageCreateInfo m_VSCInfo = {};
	VkPipelineShaderStageCreateInfo m_HSCInfo = {};
	VkPipelineShaderStageCreateInfo m_DSCInfo = {};
	VkPipelineShaderStageCreateInfo m_GSCInfo = {};
	VkPipelineShaderStageCreateInfo m_PSCInfo = {};
	VkPipelineShaderStageCreateInfo m_CSCInfo = {};
};
#pragma once
#include "ShaderProgramComponent.h"
#include "vulkan/vulkan.h"

class VKShaderProgramComponent : public ShaderProgramComponent
{
public:
	VKShaderProgramComponent() {};
	~VKShaderProgramComponent() {};

	VkShaderModule m_vertexShaderModule;
	VkShaderModule m_fragmentShaderModule;
	VkPipelineShaderStageCreateInfo m_vertexShaderStageCInfo = {};
	VkPipelineShaderStageCreateInfo m_fragmentShaderStageCInfo = {};
	VkPipelineVertexInputStateCreateInfo m_vertexInputStateCInfo = {};
};
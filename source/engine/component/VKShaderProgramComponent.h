#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"

class VKShaderProgramComponent
{
public:
	VKShaderProgramComponent() {};
	~VKShaderProgramComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	VkShaderModule m_vertexShaderModule;
	VkShaderModule m_fragmentShaderModule;
	VkPipelineShaderStageCreateInfo m_vertexShaderStageCInfo = {};
	VkPipelineShaderStageCreateInfo m_fragmentShaderStageCInfo = {};
	VkPipelineVertexInputStateCreateInfo m_vertexInputStateCInfo = {};
};
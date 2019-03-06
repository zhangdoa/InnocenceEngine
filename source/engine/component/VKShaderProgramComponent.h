#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"

class VKShaderProgramComponent
{
public:
	VKShaderProgramComponent() {};
	~VKShaderProgramComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	VkShaderModule m_vertexShaderModule;
	VkShaderModule m_fragmentShaderModule;
	VkPipelineShaderStageCreateInfo m_vertexShaderStageCreateInfo;
	VkPipelineShaderStageCreateInfo m_fragmentShaderStageCreateInfo;
	VkPipelineVertexInputStateCreateInfo m_vertexInputStateCreateInfo;
	VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyStateCreateInfo;
};
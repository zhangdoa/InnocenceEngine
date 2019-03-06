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

	VkShaderModule m_shaderModule;
	VkPipelineShaderStageCreateInfo m_vertexShaderStageCreateInfo;
	VkPipelineShaderStageCreateInfo m_fragmentShaderStageCreateInfo;
	VkPipelineVertexInputStateCreateInfo m_vertexInputStateCreateInfo;
	VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyStateCreateInfo;
};
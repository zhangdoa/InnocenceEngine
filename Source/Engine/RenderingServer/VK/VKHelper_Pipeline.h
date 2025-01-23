#pragma once
#include "../../Common/STL17.h"
#include "../../Common/LogService.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../IRenderingServer.h"

namespace Inno
{
	namespace VKHelper
	{
		VkCompareOp GetComparisionFunctionEnum(ComparisionFunction comparisionFunction);
		VkStencilOp GetStencilOperationEnum(StencilOperation stencilOperation);
		VkBlendFactor GetBlendFactorEnum(BlendFactor blendFactor);
		VkBlendOp GetBlendOperation(BlendOperation blendOperation);

		VkPipelineStageFlags GetPipelineStageFlags(const VkImageLayout& imageLayout, ShaderStage shaderStage);
	}
}
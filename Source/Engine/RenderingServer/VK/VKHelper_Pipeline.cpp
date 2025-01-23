#include "VKHelper_Pipeline.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"

#include "../../Engine.h"

using namespace Inno;

VkPipelineStageFlags VKHelper::GetPipelineStageFlags(const VkImageLayout& imageLayout, ShaderStage shaderStage)
{
	if (imageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	if (imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	if(imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		if(shaderStage == ShaderStage::Compute)
			return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	if(imageLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		if(shaderStage == ShaderStage::Compute)
			return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	if(imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	if(imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}

	return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

VkCompareOp VKHelper::GetComparisionFunctionEnum(ComparisionFunction comparisionFunction)
{
	VkCompareOp l_result;

	switch (comparisionFunction)
	{
	case ComparisionFunction::Never:
		l_result = VkCompareOp::VK_COMPARE_OP_NEVER;
		break;
	case ComparisionFunction::Less:
		l_result = VkCompareOp::VK_COMPARE_OP_LESS;
		break;
	case ComparisionFunction::Equal:
		l_result = VkCompareOp::VK_COMPARE_OP_EQUAL;
		break;
	case ComparisionFunction::LessEqual:
		l_result = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	case ComparisionFunction::Greater:
		l_result = VkCompareOp::VK_COMPARE_OP_GREATER;
		break;
	case ComparisionFunction::NotEqual:
		l_result = VkCompareOp::VK_COMPARE_OP_NOT_EQUAL;
		break;
	case ComparisionFunction::GreaterEqual:
		l_result = VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL;
		break;
	case ComparisionFunction::Always:
		l_result = VkCompareOp::VK_COMPARE_OP_ALWAYS;
		break;
	default:
		break;
	}

	return l_result;
}

VkStencilOp VKHelper::GetStencilOperationEnum(StencilOperation stencilOperation)
{
	VkStencilOp l_result;

	switch (stencilOperation)
	{
	case StencilOperation::Keep:
		l_result = VkStencilOp::VK_STENCIL_OP_KEEP;
		break;
	case StencilOperation::Zero:
		l_result = VkStencilOp::VK_STENCIL_OP_ZERO;
		break;
	case StencilOperation::Replace:
		l_result = VkStencilOp::VK_STENCIL_OP_REPLACE;
		break;
	case StencilOperation::IncreaseSat:
		l_result = VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_WRAP;
		break;
	case StencilOperation::DecreaseSat:
		l_result = VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_WRAP;
		break;
	case StencilOperation::Invert:
		l_result = VkStencilOp::VK_STENCIL_OP_INVERT;
		break;
	case StencilOperation::Increase:
		l_result = VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		break;
	case StencilOperation::Decrease:
		l_result = VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		break;
	default:
		break;
	}

	return l_result;
}

VkBlendFactor VKHelper::GetBlendFactorEnum(BlendFactor blendFactor)
{
	VkBlendFactor l_result;

	switch (blendFactor)
	{
	case BlendFactor::Zero:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
		break;
	case BlendFactor::One:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE;
		break;
	case BlendFactor::SrcColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;
		break;
	case BlendFactor::OneMinusSrcColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case BlendFactor::SrcAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
		break;
	case BlendFactor::OneMinusSrcAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case BlendFactor::DestColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;
		break;
	case BlendFactor::OneMinusDestColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case BlendFactor::DestAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;
		break;
	case BlendFactor::OneMinusDestAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case BlendFactor::Src1Color:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC1_COLOR;
		break;
	case BlendFactor::OneMinusSrc1Color:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		break;
	case BlendFactor::Src1Alpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC1_ALPHA;
		break;
	case BlendFactor::OneMinusSrc1Alpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		break;
	default:
		break;
	}

	return l_result;
}

VkBlendOp VKHelper::GetBlendOperation(BlendOperation blendOperation)
{
	VkBlendOp l_result;

	switch (blendOperation)
	{
	case BlendOperation::Add:
		l_result = VkBlendOp::VK_BLEND_OP_ADD;
		break;
	case BlendOperation::Substruct:
		l_result = VkBlendOp::VK_BLEND_OP_SUBTRACT;
		break;
	default:
		break;
	}

	return l_result;
}
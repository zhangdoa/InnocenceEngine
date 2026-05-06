#include "VKGraphicsService.h"
#include "../GraphicsResourceService.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
#include "VKHelper_Texture.h"
#include "VKHelper_Pipeline.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

bool VKGraphicsService::GenerateViewportState(ViewportDesc viewportDesc, VKPipelineStateObject *PSO)
{
	PSO->m_ViewportStateCInfo = {};
	PSO->m_ViewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	PSO->m_ViewportStateCInfo.viewportCount = 1;
	PSO->m_ViewportStateCInfo.pViewports = &PSO->m_Viewport;
	PSO->m_ViewportStateCInfo.scissorCount = 1;
	PSO->m_ViewportStateCInfo.pScissors = &PSO->m_Scissor;

	return true;
}

bool VKGraphicsService::GenerateRasterizerState(RasterizerDesc rasterizerDesc, VKPipelineStateObject *PSO)
{
	PSO->m_InputAssemblyStateCInfo = {};
	PSO->m_InputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	switch (rasterizerDesc.m_PrimitiveTopology)
	{
	case PrimitiveTopology::Point:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		break;
	case PrimitiveTopology::Line:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		break;
	case PrimitiveTopology::TriangleList:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	case PrimitiveTopology::TriangleStrip:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		break;
	case PrimitiveTopology::Patch:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		break;
	default:
		break;
	}
	PSO->m_InputAssemblyStateCInfo.primitiveRestartEnable = false;

	PSO->m_RasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	if (rasterizerDesc.m_UseCulling)
	{
		switch (rasterizerDesc.m_RasterizerCullMode)
		{
		case RasterizerCullMode::Back:
			PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		case RasterizerCullMode::Front:
			PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		default:
			break;
		}
	}
	else
	{
		PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_NONE;
	}

	switch (rasterizerDesc.m_RasterizerFillMode)
	{
	case Type::RasterizerFillMode::Point:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_POINT;
		break;
	case Type::RasterizerFillMode::Wireframe:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_LINE;
		break;
	case Type::RasterizerFillMode::Solid:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
		break;
	default:
		break;
	}

	PSO->m_RasterizationStateCInfo.frontFace = (rasterizerDesc.m_RasterizerFaceWinding == RasterizerFaceWinding::CCW) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
	PSO->m_RasterizationStateCInfo.lineWidth = 1.0f;

	PSO->m_MultisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	PSO->m_MultisampleStateCInfo.sampleShadingEnable = rasterizerDesc.m_AllowMultisample;
	PSO->m_MultisampleStateCInfo.rasterizationSamples = rasterizerDesc.m_AllowMultisample ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;

	return true;
}

bool VKGraphicsService::GenerateDepthStencilState(DepthStencilDesc depthStencilDesc, VKPipelineStateObject *PSO)
{
	PSO->m_DepthStencilStateCInfo = {};
	PSO->m_DepthStencilStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	PSO->m_DepthStencilStateCInfo.depthTestEnable = depthStencilDesc.m_DepthEnable;
	PSO->m_DepthStencilStateCInfo.depthWriteEnable = depthStencilDesc.m_AllowDepthWrite;
	PSO->m_DepthStencilStateCInfo.depthCompareOp = GetComparisionFunctionEnum(depthStencilDesc.m_DepthComparisionFunction);
	PSO->m_DepthStencilStateCInfo.depthBoundsTestEnable = VK_FALSE;
	PSO->m_DepthStencilStateCInfo.minDepthBounds = 0.0f; // Optional
	PSO->m_DepthStencilStateCInfo.maxDepthBounds = 1.0f; // Optional

	PSO->m_DepthStencilStateCInfo.stencilTestEnable = depthStencilDesc.m_StencilEnable;

	PSO->m_DepthStencilStateCInfo.front.failOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilFailOperation);
	PSO->m_DepthStencilStateCInfo.front.passOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilPassOperation);
	PSO->m_DepthStencilStateCInfo.front.depthFailOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilStateCInfo.front.compareOp = GetComparisionFunctionEnum(depthStencilDesc.m_FrontFaceStencilComparisionFunction);
	PSO->m_DepthStencilStateCInfo.front.compareMask = 0xFF;
	PSO->m_DepthStencilStateCInfo.front.writeMask = depthStencilDesc.m_StencilWriteMask;
	PSO->m_DepthStencilStateCInfo.front.reference = depthStencilDesc.m_StencilReference;

	PSO->m_DepthStencilStateCInfo.back.failOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilFailOperation);
	PSO->m_DepthStencilStateCInfo.back.passOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilPassOperation);
	PSO->m_DepthStencilStateCInfo.back.depthFailOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilStateCInfo.back.compareOp = GetComparisionFunctionEnum(depthStencilDesc.m_BackFaceStencilComparisionFunction);
	PSO->m_DepthStencilStateCInfo.back.compareMask = 0xFF;
	PSO->m_DepthStencilStateCInfo.back.writeMask = depthStencilDesc.m_StencilWriteMask;
	PSO->m_DepthStencilStateCInfo.back.reference = depthStencilDesc.m_StencilReference;

	return true;
}

bool VKGraphicsService::GenerateBlendState(BlendDesc blendDesc, size_t colorBlendAttachmentCount, VKPipelineStateObject *PSO)
{
	PSO->m_ColorBlendStateCInfo = {};
	PSO->m_ColorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineColorBlendAttachmentState l_colorBlendAttachmentState = {};
	l_colorBlendAttachmentState.blendEnable = blendDesc.m_UseBlend;
	l_colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_colorBlendAttachmentState.srcColorBlendFactor = GetBlendFactorEnum(blendDesc.m_SourceRGBFactor);
	l_colorBlendAttachmentState.srcAlphaBlendFactor = GetBlendFactorEnum(blendDesc.m_SourceAlphaFactor);
	l_colorBlendAttachmentState.dstColorBlendFactor = GetBlendFactorEnum(blendDesc.m_DestinationRGBFactor);
	l_colorBlendAttachmentState.dstAlphaBlendFactor = GetBlendFactorEnum(blendDesc.m_DestinationAlphaFactor);
	l_colorBlendAttachmentState.colorBlendOp = GetBlendOperation(blendDesc.m_BlendOperation);
	l_colorBlendAttachmentState.alphaBlendOp = GetBlendOperation(blendDesc.m_BlendOperation);

	PSO->m_ColorBlendAttachmentStates.reserve(colorBlendAttachmentCount);

	for (size_t i = 0; i < colorBlendAttachmentCount; i++)
	{
		PSO->m_ColorBlendAttachmentStates.emplace_back(l_colorBlendAttachmentState);
	}

	PSO->m_ColorBlendStateCInfo.logicOpEnable = VK_FALSE;
	PSO->m_ColorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;

	PSO->m_ColorBlendStateCInfo.blendConstants[0] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[1] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[2] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[3] = 0.0f;

	if (PSO->m_ColorBlendAttachmentStates.size())
	{
		PSO->m_ColorBlendStateCInfo.attachmentCount = (uint32_t)PSO->m_ColorBlendAttachmentStates.size();
		PSO->m_ColorBlendStateCInfo.pAttachments = &PSO->m_ColorBlendAttachmentStates[0];
	}

	return true;
}

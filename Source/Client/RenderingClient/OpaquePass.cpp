#include "OpaquePass.h"
#include "DefaultGPUBuffers.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace OpaquePass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool OpaquePass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("OpaquePass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "opaquePass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "opaquePass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("OpaquePass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 4;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(9);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 2;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_GlobalSlot = 6;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_LocalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_GlobalSlot = 7;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_LocalSlot = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_GlobalSlot = 8;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_IsRanged = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("OpaquePass/");

	m_SDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool OpaquePass::Initialize()
{
	return true;
}

bool OpaquePass::PrepareCommandList()
{
	auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 8, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_CameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly, false, 0, l_CameraGBDC->m_TotalSize);

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_opaquePassGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassGPUData()[i];

		if (l_opaquePassGPUData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, true, l_offset, l_MeshGBDC->m_ElementSize);
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, true, l_offset, l_MaterialGBDC->m_ElementSize);

			if (l_opaquePassGPUData.material->m_objectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[0], 3, 0);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[1], 4, 1);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[2], 5, 2);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[3], 6, 3);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[4], 7, 4);
			}

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_opaquePassGPUData.mesh);

			if (l_opaquePassGPUData.material->m_objectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[0], 3, 0);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[1], 4, 1);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[2], 5, 2);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[3], 6, 3);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_opaquePassGPUData.material->m_ResourceBinders[4], 7, 4);
			}
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool OpaquePass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

RenderPassDataComponent * OpaquePass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * OpaquePass::GetSPC()
{
	return m_SPC;
}
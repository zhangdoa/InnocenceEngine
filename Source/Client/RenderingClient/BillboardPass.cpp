#include "BillboardPass.h"
#include "DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace BillboardPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool BillboardPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("BillboardPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "billboardPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "billboardPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("BillboardPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 12;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("BillboardPass/");

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool BillboardPass::Initialize()
{
	return true;
}

bool BillboardPass::PrepareCommandList()
{
	auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
	auto l_BillboardGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Billboard);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->CopyDepthBuffer(OpaquePass::GetRPDC(), m_RPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 3, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_CameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Quad);

	auto l_billboardPassDrawCallData = g_pModuleManager->getRenderingFrontend()->getBillboardPassDrawCallData();
	auto l_drawCallCount = l_billboardPassDrawCallData.size();

	for (unsigned int i = 0; i < l_drawCallCount; i++)
	{
		auto l_iconTexture = l_billboardPassDrawCallData[i].iconTexture;
		auto l_offset = l_billboardPassDrawCallData[i].meshGPUDataOffset;
		auto l_instanceCount = l_billboardPassDrawCallData[i].instanceCount;

		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_BillboardGBDC->m_ResourceBinder, 1, 12, Accessibility::ReadOnly, l_offset, l_instanceCount);

		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_iconTexture->m_ResourceBinder, 2, 0);

		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh, l_instanceCount);

		g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_iconTexture->m_ResourceBinder, 2, 0);
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool BillboardPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool BillboardPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent * BillboardPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * BillboardPass::GetSPC()
{
	return m_SPC;
}
#include "TransparentPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace TransparentPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
}

bool TransparentPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("TransparentPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "transparentPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "transparentPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("TransparentPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc.m_UseBlend = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc.m_SourceRGBFactor = BlendFactor::SrcAlpha;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc.m_SourceAlphaFactor = BlendFactor::One;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc.m_DestinationRGBFactor = BlendFactor::Src1Color;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc.m_DestinationAlphaFactor = BlendFactor::Zero;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 2;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 3;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	return true;
}

bool TransparentPass::Initialize()
{
	return true;
}

bool TransparentPass::PrepareCommandList()
{
	auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::TransparentPassMesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::TransparentPassMaterial);
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->CopyDepthStencilBuffer(OpaquePass::GetRPDC(), m_RPDC);
	g_pModuleManager->getRenderingServer()->CopyColorBuffer(PreTAAPass::GetRPDC(), 0, m_RPDC, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_SunGBDC->m_ResourceBinder, 3, 3, Accessibility::ReadOnly);

	uint32_t l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getTransparentPassDrawCallCount();
	auto& l_transparentPassDrawCallData = g_pModuleManager->getRenderingFrontend()->getTransparentPassDrawCallData();

	for (uint32_t i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_drawCallData = l_transparentPassDrawCallData[i];
		if (l_drawCallData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshGPUDataIndex, 1);
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialGPUDataIndex, 1);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_drawCallData.mesh);
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool TransparentPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool TransparentPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent * TransparentPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * TransparentPass::GetSPC()
{
	return m_SPC;
}
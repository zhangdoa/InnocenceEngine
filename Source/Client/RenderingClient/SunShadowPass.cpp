#include "SunShadowPass.h"
#include "DefaultGPUBuffers.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace SunShadowPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
}

bool SunShadowPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "sunShadowPass.vert/";
	m_SPC->m_ShaderFilePaths.m_GSPath = "sunShadowPass.geom/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "sunShadowPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("SunShadowPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 2048;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 2048;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerCullMode = RasterizerCullMode::Front;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 1;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 6;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	return true;
}

bool SunShadowPass::Initialize()
{
	return true;
}

bool SunShadowPass::PrepareCommandList()
{
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Geometry, l_CSMGBDC->m_ResourceBinder, 1, 6, Accessibility::ReadOnly, false, 0, l_CSMGBDC->m_TotalSize);

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_opaquePassGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassGPUData()[i];

		if (l_opaquePassGPUData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 0, 1, Accessibility::ReadOnly, true, l_offset, l_MeshGBDC->m_ElementSize);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_opaquePassGPUData.mesh);
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

RenderPassDataComponent * SunShadowPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * SunShadowPass::GetSPC()
{
	return m_SPC;
}
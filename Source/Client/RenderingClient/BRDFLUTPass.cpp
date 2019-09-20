#include "BRDFLUTPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "LightPass.h"
#include "SkyPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace BRDFLUTPass
{
	RenderPassDataComponent* m_RPDC;
	RenderPassDataComponent* m_RPDC_MS;
	ShaderProgramComponent* m_SPC;
	ShaderProgramComponent* m_SPC_MS;
	SamplerDataComponent* m_SDC;
}

bool BRDFLUTPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("BRDFLUTPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "BRDFLUTPass.frag/";

	m_SPC_MS = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("BRDFLUTMSPass/");

	m_SPC_MS->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC_MS->m_ShaderFilePaths.m_PSPath = "BRDFLUTMSPass.frag/";

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("BRDFLUTPass/");
	m_RPDC_MS = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("BRDFLUTMSPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 512;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 512;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;
	m_RPDC_MS->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_MS->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[0].m_IsRanged = true;

	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 0;
	m_RPDC_MS->m_ResourceBinderLayoutDescs[1].m_IsRanged = true;

	m_RPDC->m_ShaderProgram = m_SPC;
	m_RPDC_MS->m_ShaderProgram = m_SPC_MS;

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("BRDFLUTMSPass/");

	return true;
}

bool BRDFLUTPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_MS);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_MS);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool BRDFLUTPass::PrepareCommandList()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Quad);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	// Multi-scattering LUT
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_MS, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_MS);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_MS);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_MS, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 1, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_MS, ShaderStage::Pixel, m_RPDC->m_RenderTargetsResourceBinders[0], 0, 0);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC_MS, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_MS, ShaderStage::Pixel, m_RPDC->m_RenderTargetsResourceBinders[0], 0, 0);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_MS);

	return true;
}

bool BRDFLUTPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_MS);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_MS);

	return true;
}

bool BRDFLUTPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_MS);

	return true;
}

IResourceBinder * BRDFLUTPass::GetBRDFLUT()
{
	return m_RPDC->m_RenderTargetsResourceBinders[0];
}

IResourceBinder * BRDFLUTPass::GetBRDFMSLUT()
{
	return m_RPDC_MS->m_RenderTargetsResourceBinders[0];
}
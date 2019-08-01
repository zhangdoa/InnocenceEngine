#include "FinalBlendPass.h"
#include "DefaultGPUBuffers.h"

#include "LightPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace FinalBlendPass
{
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool FinalBlendPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("FinalBlendPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "finalBlendPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	auto l_RPDC = g_pModuleManager->getRenderingServer()->GetSwapChainRPDC();

	l_RPDC->m_ResourceBinderLayoutDescs.resize(2);
	l_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	l_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	l_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceCount = 3;
	l_RPDC->m_ResourceBinderLayoutDescs[0].m_IsRanged = true;

	l_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Sampler;
	l_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	l_RPDC->m_ResourceBinderLayoutDescs[1].m_IsRanged = true;

	l_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("FinalBlendPass/");

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool FinalBlendPass::Initialize()
{
	return true;
}

bool FinalBlendPass::PrepareCommandList()
{
	auto l_RPDC = g_pModuleManager->getRenderingServer()->GetSwapChainRPDC();

	auto l_currentFrame = l_RPDC->m_CurrentFrame;

	g_pModuleManager->getRenderingServer()->CommandListBegin(l_RPDC, l_currentFrame);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(l_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(l_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(l_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 1, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(l_RPDC, ShaderStage::Pixel, LightPass::GetRPDC()->m_RenderTargetsResourceBinder, 0, 0);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Quad);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(l_RPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(l_RPDC, ShaderStage::Pixel, LightPass::GetRPDC()->m_RenderTargetsResourceBinder, 0, 0);

	g_pModuleManager->getRenderingServer()->CommandListEnd(l_RPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(l_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(l_RPDC);

	return true;
}

ShaderProgramComponent * FinalBlendPass::getSPC()
{
	return m_SPC;
}
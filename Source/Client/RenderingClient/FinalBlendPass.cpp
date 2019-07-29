#include "FinalBlendPass.h"
#include "DefaultGPUBuffers.h"

#include "LightPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace FinalBlendPass
{
	ShaderProgramComponent* m_SPC;
}

bool FinalBlendPass::Initialize()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("FinalBlendPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_FSPath = "finalBlendPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	return true;
}

bool FinalBlendPass::PrepareCommandList()
{
	auto l_RPC = g_pModuleManager->getRenderingServer()->GetSwapChainRPC();

	g_pModuleManager->getRenderingServer()->CommandListBegin(l_RPC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(l_RPC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(l_RPC);
	g_pModuleManager->getRenderingServer()->BindShaderProgramComponent(m_SPC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(ShaderType::FRAGMENT, LightPass::getRPC()->m_RenderTargetsResourceBinder, 0);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::QUAD);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(l_RPC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(ShaderType::FRAGMENT, LightPass::getRPC()->m_RenderTargetsResourceBinder, 0);

	g_pModuleManager->getRenderingServer()->CommandListEnd(l_RPC, 0);

	return true;
}

ShaderProgramComponent * FinalBlendPass::getSPC()
{
	return m_SPC;
}
#include "LightPass.h"
#include "DefaultGPUBuffers.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace LightPass
{
	RenderPassDataComponent* m_RPC;
	ShaderProgramComponent* m_SPC;
}

bool LightPass::Initialize()
{
	m_RPC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("LightPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPC->m_RenderPassDesc = l_RenderPassDesc;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPC);

	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("LightPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_FSPath = "lightPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	return true;
}

bool LightPass::PrepareCommandList()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPC);
	g_pModuleManager->getRenderingServer()->BindShaderProgramComponent(m_SPC);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::QUAD);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPC, l_mesh);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPC, 0);

	return true;
}

RenderPassDataComponent * LightPass::getRPC()
{
	return m_RPC;
}

ShaderProgramComponent * LightPass::getSPC()
{
	return m_SPC;
}
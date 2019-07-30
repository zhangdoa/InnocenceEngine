#include "LightPass.h"
#include "DefaultGPUBuffers.h"

#include "OpaquePass.h"

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
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("LightPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_FSPath = "lightPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("LightPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Equal;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Equal;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPC->m_ResourceBinderLayoutDescs.resize(6);
	m_RPC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::ROBuffer;
	m_RPC->m_ResourceBinderLayoutDescs[0].m_BindingSlot = 0;

	m_RPC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::ROBuffer;
	m_RPC->m_ResourceBinderLayoutDescs[1].m_BindingSlot = 3;

	m_RPC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPC->m_ResourceBinderLayoutDescs[2].m_BindingSlot = 4;

	m_RPC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPC->m_ResourceBinderLayoutDescs[3].m_BindingSlot = 5;

	m_RPC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPC->m_ResourceBinderLayoutDescs[4].m_BindingSlot = 0;
	m_RPC->m_ResourceBinderLayoutDescs[4].m_ResourceCount = 4;
	m_RPC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_RPC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPC->m_ResourceBinderLayoutDescs[5].m_BindingSlot = 0;
	m_RPC->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_RPC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPC);

	return true;
}

bool LightPass::PrepareCommandList()
{
	auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);
	auto l_PointLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PointLight);
	auto l_SphereLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::SphereLight);
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);

	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, l_CameraGBDC, 0, l_CameraGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, l_SunGBDC, 0, l_SunGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, l_PointLightGBDC, 0, l_PointLightGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, l_SphereLightGBDC, 0, l_SphereLightGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, l_SkyGBDC, 0, l_SkyGBDC->m_TotalSize);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPC);
	g_pModuleManager->getRenderingServer()->BindShaderProgramComponent(m_SPC);

	g_pModuleManager->getRenderingServer()->CopyStencilBuffer(OpaquePass::getRPC(), m_RPC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(ShaderType::FRAGMENT, OpaquePass::getRPC()->m_RenderTargetsResourceBinder, 0);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::QUAD);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(ShaderType::FRAGMENT, OpaquePass::getRPC()->m_RenderTargetsResourceBinder, 0);

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
#include "LightPass.h"
#include "DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace LightPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool LightPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("LightPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_FSPath = "lightPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("LightPass/");

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

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(10);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::ROBuffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_BindingSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_BindingSlot = 1;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_BindingSlot = 2;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::ROBuffer;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_BindingSlot = 3;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_BindingSlot = 4;

	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_BindingSlot = 5;

	m_RPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_BindingSlot = 6;

	m_RPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::ROBuffer;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_BindingSlot = 7;

	m_RPDC->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_BindingSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_ResourceCount = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_BindingSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_IsRanged = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("LightPass/");

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool LightPass::Initialize()
{
	return true;
}

bool LightPass::PrepareCommandList()
{
	auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);
	auto l_PointLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PointLight);
	auto l_SphereLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::SphereLight);
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->BindShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderType::FRAGMENT, m_SDC->m_ResourceBinder, 9);

	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(m_RPDC, l_CameraGBDC, ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, 0, l_CameraGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(m_RPDC, l_SunGBDC, ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, 0, l_SunGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(m_RPDC, l_PointLightGBDC, ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, 0, l_PointLightGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(m_RPDC, l_SphereLightGBDC, ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, 0, l_SphereLightGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(m_RPDC, l_SkyGBDC, ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, 0, l_SkyGBDC->m_TotalSize);

	g_pModuleManager->getRenderingServer()->CopyStencilBuffer(OpaquePass::GetRPDC(), m_RPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderType::FRAGMENT, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinder, 8);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::QUAD);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderType::FRAGMENT, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinder, 8);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

RenderPassDataComponent * LightPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * LightPass::GetSPC()
{
	return m_SPC;
}
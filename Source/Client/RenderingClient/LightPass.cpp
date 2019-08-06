#include "LightPass.h"
#include "DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "BRDFLUTPass.h"
#include "SSAOPAss.h"
#include "SunShadowPass.h"

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
	m_SPC->m_ShaderFilePaths.m_PSPath = "lightPass.frag/";

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

	m_RPDC->m_ResourceBinderLayoutDescs.resize(15);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 3;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 4;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 5;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 6;

	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 7;

	m_RPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_GlobalSlot = 6;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_GlobalSlot = 7;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_LocalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_GlobalSlot = 8;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_LocalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_GlobalSlot = 9;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_LocalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[10].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_GlobalSlot = 10;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_LocalSlot = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[11].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_GlobalSlot = 11;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_LocalSlot = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[12].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[12].m_GlobalSlot = 12;
	m_RPDC->m_ResourceBinderLayoutDescs[12].m_LocalSlot = 6;
	m_RPDC->m_ResourceBinderLayoutDescs[12].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[13].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_GlobalSlot = 13;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_LocalSlot = 7;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[14].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_GlobalSlot = 14;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_IsRanged = true;

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
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 14, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_CameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly, false, 0, l_CameraGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_SunGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly, false, 0, l_SunGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_PointLightGBDC->m_ResourceBinder, 2, 4, Accessibility::ReadOnly, false, 0, l_PointLightGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_SphereLightGBDC->m_ResourceBinder, 3, 5, Accessibility::ReadOnly, false, 0, l_SphereLightGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_CSMGBDC->m_ResourceBinder, 4, 6, Accessibility::ReadOnly, false, 0, l_CSMGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_SkyGBDC->m_ResourceBinder, 5, 7, Accessibility::ReadOnly, false, 0, l_SkyGBDC->m_TotalSize);

	g_pModuleManager->getRenderingServer()->CopyStencilBuffer(OpaquePass::GetRPDC(), m_RPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 6, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[1], 7, 1);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[2], 8, 2);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 9, 3);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 10, 4);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 11, 5);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, SSAOPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 12, 6);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, SunShadowPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 13, 7);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Quad);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 6, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[1], 7, 1);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[2], 8, 2);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 9, 3);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 10, 4);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 11, 5);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, SSAOPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 12, 6);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, SunShadowPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 13, 7);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool LightPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool LightPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

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
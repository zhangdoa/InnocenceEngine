#include "LightPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "BRDFLUTPass.h"
#include "SSAOPass.h"
#include "SunShadowPass.h"
#include "LightCullingPass.h"
#include "GIResolvePass.h"
#include "VolumetricFogPass.h"

#include "../../Engine/Interface/IModuleManager.h"

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

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("LightPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Equal;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Equal;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(18);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 3;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 4;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 5;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 8;

	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorIndex = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorIndex = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[10].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorIndex = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[11].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_DescriptorIndex = 6;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[12].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[12].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[12].m_DescriptorIndex = 7;
	m_RPDC->m_ResourceBinderLayoutDescs[12].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[13].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_DescriptorIndex = 8;
	m_RPDC->m_ResourceBinderLayoutDescs[13].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[14].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_DescriptorIndex = 9;
	m_RPDC->m_ResourceBinderLayoutDescs[14].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[15].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[15].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[15].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[15].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[15].m_DescriptorIndex = 10;
	m_RPDC->m_ResourceBinderLayoutDescs[15].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[16].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[16].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[16].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[16].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[16].m_DescriptorIndex = 11;

	m_RPDC->m_ResourceBinderLayoutDescs[17].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[17].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[17].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[17].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("LightPass/");

	return true;
}

bool LightPass::Initialize()
{
	m_RPDC->m_DepthStencilRenderTarget = OpaquePass::GetRPDC()->m_DepthStencilRenderTarget;

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool LightPass::PrepareCommandList()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PointLight);
	auto l_SphereLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::SphereLight);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);
	auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 17, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_PointLightGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_SphereLightGBDC->m_ResourceBinder, 2, 4, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_CSMGBDC->m_ResourceBinder, 3, 5, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_GIGBDC->m_ResourceBinder, 4, 8, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 5, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[1], 6, 1);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[2], 7, 2);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 8, 3);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 9, 4);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 10, 5);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, SSAOPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 11, 6);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, SunShadowPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 12, 7);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, LightCullingPass::GetLightGrid(), 13, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, GIResolvePass::GetIrradianceVolume(), 14, 9, Accessibility::ReadOnly);
	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, VolumetricFogPass::GetRayMarchingResult(), 15, 10, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, LightCullingPass::GetLightIndexList()->m_ResourceBinder, 16, 11, Accessibility::ReadOnly, 0);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Square);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 5, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[1], 6, 1);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[2], 7, 2);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 8, 3);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 9, 4);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 10, 5);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, SSAOPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 11, 6);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, SunShadowPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 12, 7);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, LightCullingPass::GetLightGrid(), 13, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, GIResolvePass::GetIrradianceVolume(), 14, 9, Accessibility::ReadOnly);
	//g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, VolumetricFogPass::GetRayMarchingResult(), 15, 10, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, LightCullingPass::GetLightIndexList()->m_ResourceBinder, 16, 11, Accessibility::ReadOnly, 0);

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

RenderPassDataComponent* LightPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* LightPass::GetSPC()
{
	return m_SPC;
}
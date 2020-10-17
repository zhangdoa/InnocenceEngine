#include "LightPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VoxelizationPass.h"
#include "OpaquePass.h"
#include "BRDFLUTPass.h"
#include "SSAOPass.h"
#include "SunShadowPass.h"
#include "LightCullingPass.h"
#include "GIResolvePass.h"
#include "VolumetricPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace LightPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	TextureDataComponent* m_TDC_0;
	TextureDataComponent* m_TDC_1;
	SamplerDataComponent* m_SDC;
}

bool LightPass::Setup()
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("LightPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "lightPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("LightPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(21);
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

	m_RPDC->m_ResourceBinderLayoutDescs[18].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[18].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[18].m_DescriptorIndex = 9;

	m_RPDC->m_ResourceBinderLayoutDescs[19].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[19].m_DescriptorSetIndex = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[19].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[19].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[19].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[19].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[20].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[20].m_DescriptorSetIndex = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[20].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[20].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[20].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[20].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("LightPass/");

	m_TDC_0 = g_Engine->getRenderingServer()->AddTextureDataComponent("LightPass0/");
	m_TDC_0->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_TDC_1 = g_Engine->getRenderingServer()->AddTextureDataComponent("LightPass1/");
	m_TDC_1->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	return true;
}

bool LightPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC_0);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC_1);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool LightPass::PrepareCommandList()
{
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PointLight);
	auto l_SphereLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::SphereLight);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);
	auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, m_SDC->m_ResourceBinder, 17, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, l_PointLightGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, l_SphereLightGBDC->m_ResourceBinder, 2, 4, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, l_CSMGBDC->m_ResourceBinder, 3, 5, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, l_GIGBDC->m_ResourceBinder, 4, 8, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, VoxelizationPass::GetVoxelizationCBuffer(), 18, 9, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 5, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[1], 6, 1);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[2], 7, 2);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 8, 3);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, BRDFLUTPass::GetBRDFLUT(), 9, 4);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, BRDFLUTPass::GetBRDFMSLUT(), 10, 5);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, SSAOPass::GetResult(), 11, 6);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, SunShadowPass::GetShadowMap(), 12, 7);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, LightCullingPass::GetLightGrid(), 13, 8, Accessibility::ReadOnly);
	//g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, GIResolvePass::GetIrradianceVolume(), 14, 9, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, VoxelizationPass::GetVoxelizationLuminanceVolume(), 14, 9, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 15, 10, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, LightCullingPass::GetLightIndexList(), 16, 11, Accessibility::ReadOnly, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC_0->m_ResourceBinder, 19, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC_1->m_ResourceBinder, 20, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->DispatchCompute(m_RPDC, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 5, 0);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[1], 6, 1);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[2], 7, 2);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 8, 3);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, BRDFLUTPass::GetBRDFLUT(), 9, 4);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, BRDFLUTPass::GetBRDFMSLUT(), 10, 5);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, SSAOPass::GetResult(), 11, 6);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, SunShadowPass::GetShadowMap(), 12, 7);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, LightCullingPass::GetLightGrid(), 13, 8, Accessibility::ReadOnly);
	//g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, GIResolvePass::GetIrradianceVolume(), 14, 9, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, VoxelizationPass::GetVoxelizationLuminanceVolume(), 14, 9, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 15, 10, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, LightCullingPass::GetLightIndexList(), 16, 11, Accessibility::ReadOnly, 0);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC_0->m_ResourceBinder, 19, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC_1->m_ResourceBinder, 20, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool LightPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

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

IResourceBinder* LightPass::GetResult(uint32_t index)
{
	if (index == 0)
	{
		return m_TDC_0->m_ResourceBinder;
	}
	else
	{
		return m_TDC_1->m_ResourceBinder;
	}
}
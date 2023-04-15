#include "LightPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VXGIRenderer.h"
#include "OpaquePass.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SSAOPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurEvenPass.h"
#include "LightCullingPass.h"
#include "GIResolvePass.h"
#include "VolumetricPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool LightPass::Setup(ISystemConfig *systemConfig)
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("LightPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "lightPass.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("LightPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(21);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 3;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 4;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 5;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 8;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex = 6;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorIndex = 7;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorIndex = 8;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorIndex = 9;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorIndex = 10;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorIndex = 11;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorIndex = 9;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("LightPass/");

	m_TextureComp_Luminance = g_Engine->getRenderingServer()->AddTextureComponent("LightPass0/");
	m_TextureComp_Luminance->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_TextureComp_Illuminance = g_Engine->getRenderingServer()->AddTextureComponent("LightPass1/");
	m_TextureComp_Illuminance->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool LightPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_TextureComp_Luminance);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_TextureComp_Illuminance);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool LightPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LightPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LightPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PointLight);
	auto l_SphereLightGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::SphereLight);
	auto l_CSMGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::CSM);
	auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 17);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_SphereLightGPUBufferComp, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_CSMGPUBufferComp, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 4, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 18, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0], 5);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[1], 6);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[2], 7);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3], 8);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTPass::Get().GetResult(), 9);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTMSPass::Get().GetResult(), 10);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SSAOPass::Get().GetResult(), 11);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 12);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 13, Accessibility::ReadOnly);
	//g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, GIResolvePass::GetIrradianceVolume(), 14, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetResult(), 14, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 15, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 16, Accessibility::ReadOnly, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp_Luminance, 19, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp_Illuminance, 20, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0], 5);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[1], 6);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[2], 7);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3], 8);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTPass::Get().GetResult(), 9);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTMSPass::Get().GetResult(), 10);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, SSAOPass::Get().GetResult(), 11);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 12);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 13, Accessibility::ReadOnly);
	//g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, GIResolvePass::GetIrradianceVolume(), 14, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetResult(), 14, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 15, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 16, Accessibility::ReadOnly, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp_Luminance, 19, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp_Illuminance, 20, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* LightPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* LightPass::GetLuminanceResult()
{
	return m_TextureComp_Luminance;
}

TextureComponent* LightPass::GetIlluminanceResult()
{
	return m_TextureComp_Illuminance;
}
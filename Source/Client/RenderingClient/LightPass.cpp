#include "LightPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "VXGIRenderer.h"
#include "OpaquePass.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SSAOPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurEvenPass.h"
#include "LightCullingPass.h"
#include "RadianceCachePass.h"
#include "VolumetricPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool LightPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("LightPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "lightPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("LightPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&LightPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(21);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// b1 - PointLightCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	// b2 - SphereLightCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	// b3 - CSMCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 3;
	
	// b4 - Voxelization CBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 4;

	// b5 - GI CBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 5;

	// t0 - World Position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ColorAttachment;

	// t1 - World Normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ColorAttachment;

	// t2 - Albedo
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_TextureUsage = TextureUsage::ColorAttachment;

	// t3 - Metallic, Roughness, AO
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_TextureUsage = TextureUsage::ColorAttachment;

	// t4 - BRDF LUT
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_TextureUsage = TextureUsage::ColorAttachment;

	// t5 - BRDF LUT MS
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_TextureUsage = TextureUsage::ColorAttachment;

	// t6 - SSAO
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorIndex = 6;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_TextureUsage = TextureUsage::ColorAttachment;

	// t7 - Sun Shadow
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorIndex = 7;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_TextureUsage = TextureUsage::ColorAttachment;

	// t8 - Light Culling Grid
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorIndex = 8;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_TextureUsage = TextureUsage::ColorAttachment;

	// t9 - Light Index List
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorIndex = 9;

	// t10 - Illuminance from Radiance Cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorIndex = 10;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_TextureUsage = TextureUsage::ColorAttachment;

	// t11 - Volumetric Fog
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorIndex = 11;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_TextureUsage = TextureUsage::ColorAttachment;

	// u0 - Luminance Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_TextureUsage = TextureUsage::ColorAttachment;

	// u1 - Illuminance Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_TextureUsage = TextureUsage::ColorAttachment;

	// s0 - Sampler
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("LightPass/");

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool LightPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LightPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_SamplerComp);
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	l_renderingServer->Delete(m_LuminanceResult);
	l_renderingServer->Delete(m_IlluminanceResult);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LightPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LightPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_LuminanceResult->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (m_IlluminanceResult->m_ObjectStatus != ObjectStatus::Activated)
		return false;	

	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_currentFrame = l_renderingServer->GetCurrentFrame();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PointLight);
	auto l_SphereLightGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::SphereLight);
	auto l_CSMGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::CSM);
	//auto l_GIGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_SphereLightGPUBufferComp, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_CSMGPUBufferComp, 3);
	//l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 4);
	//l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 5);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 6);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 7);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[2], 8);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3], 9);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTPass::Get().GetResult(), 10);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTMSPass::Get().GetResult(), 11);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SSAOPass::Get().GetResult(), 12);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 13);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 14);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 15);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, RadianceCachePass::Get().GetResult(), 16);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetResult(), 16);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 17);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_LuminanceResult, 18);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_IlluminanceResult, 19);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 20);

	l_renderingServer->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* LightPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* LightPass::GetLuminanceResult()
{
	return m_LuminanceResult;
}

TextureComponent* LightPass::GetIlluminanceResult()
{
	return m_IlluminanceResult;
}

bool LightPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_LuminanceResult)
		l_renderingServer->Delete(m_LuminanceResult);

	if (m_IlluminanceResult)
		l_renderingServer->Delete(m_IlluminanceResult);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	m_LuminanceResult = l_renderingServer->AddTextureComponent("LightPass Luminance Result/");
	m_LuminanceResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_LuminanceResult->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_renderingServer->Initialize(m_LuminanceResult);

	m_IlluminanceResult = l_renderingServer->AddTextureComponent("LightPass Illuminance Result/");
	m_IlluminanceResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_IlluminanceResult->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_renderingServer->Initialize(m_IlluminanceResult);

	return true;
}
#include "LightPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingFrontend.h"

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

#include "../../Engine/Engine.h"

using namespace Inno;


using namespace DefaultGPUBuffers;

bool LightPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("LightPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "lightPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("LightPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingFrontend>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 2;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

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

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("LightPass/");

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool LightPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeSamplerComponent(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool LightPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LightPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LightPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_viewportSize = g_Engine->Get<RenderingFrontend>()->GetScreenResolution();

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PointLight);
	auto l_SphereLightGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::SphereLight);
	auto l_CSMGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::CSM);
	auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 17);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_SphereLightGPUBufferComp, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_CSMGPUBufferComp, 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 18);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture, 5);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[1].m_Texture, 6);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[2].m_Texture, 7);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3].m_Texture, 8);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTPass::Get().GetResult(), 9);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTMSPass::Get().GetResult(), 10);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SSAOPass::Get().GetResult(), 11);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 12);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 13);
	//l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, GIResolvePass::GetIrradianceVolume(), 14);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetResult(), 14);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 15);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 16, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0].m_Texture, 19);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[1].m_Texture, 20);

	l_renderingServer->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture, 5);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[1].m_Texture, 6);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[2].m_Texture, 7);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3].m_Texture, 8);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTPass::Get().GetResult(), 9);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, BRDFLUTMSPass::Get().GetResult(), 10);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, SSAOPass::Get().GetResult(), 11);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 12);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 13);
	//l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, GIResolvePass::GetIrradianceVolume(), 14);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetResult(), 14);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 15);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 16);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0].m_Texture, 19);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[1].m_Texture, 20);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* LightPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* LightPass::GetLuminanceResult()
{
	return m_RenderPassComp->m_RenderTargets[0].m_Texture;
}

TextureComponent* LightPass::GetIlluminanceResult()
{
	return m_RenderPassComp->m_RenderTargets[1].m_Texture;
}
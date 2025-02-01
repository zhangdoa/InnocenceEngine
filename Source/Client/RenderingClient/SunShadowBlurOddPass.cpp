#include "SunShadowBlurOddPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "SunShadowGeometryProcessPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;




bool SunShadowBlurOddPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_shadowMapResolution = SunShadowGeometryProcessPass::Get().GetShadowMapResolution();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_Resizable = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("SunShadowBlurOddPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "sunShadowBlurPassOdd.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("SunShadowBlurOddPass/");

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs = m_RenderPassComp->m_ResourceBindingLayoutDescs;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_numThreads = TVec4<uint32_t>(16, 16, 1, 0);
	m_numThreadGroups = TVec4<uint32_t>(l_shadowMapResolution / 16, l_shadowMapResolution / 16, 1, 0);

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowBlurOddPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool SunShadowBlurOddPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowBlurOddPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowBlurOddPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_shadowMapResolution = SunShadowGeometryProcessPass::Get().GetShadowMapResolution();	
	auto l_perFrameGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	// l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_RenderPassComp);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_perFrameGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture, 1);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0].m_Texture, 2);

	// l_renderingServer->Dispatch(m_RenderPassComp, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture, 1);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0].m_Texture, 2);

	// l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* SunShadowBlurOddPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SunShadowBlurOddPass::GetResult()
{
	return m_RenderPassComp->m_RenderTargets[0].m_Texture;
}
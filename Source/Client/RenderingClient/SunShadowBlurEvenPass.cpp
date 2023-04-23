#include "SunShadowBlurEvenPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurOddPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool SunShadowBlurEvenPass::Setup(ISystemConfig *systemConfig)
{
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();
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

	m_ShaderProgramComp = g_Engine->getRenderingServer()->AddShaderProgramComponent("SunShadowBlurEvenPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "sunShadowBlurPassEven.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("SunShadowBlurEvenPass/");

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

bool SunShadowBlurEvenPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_ShaderProgramComp);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool SunShadowBlurEvenPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowBlurEvenPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowBlurEvenPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_shadowMapResolution = SunShadowGeometryProcessPass::Get().GetShadowMapResolution();	
	auto l_perFrameGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_perFrameGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowBlurOddPass::Get().GetResult(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0], 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, SunShadowBlurOddPass::Get().GetResult(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RenderPassComp->m_RenderTargets[0], 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* SunShadowBlurEvenPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SunShadowBlurEvenPass::GetResult()
{
	return m_RenderPassComp->m_RenderTargets[0];
}
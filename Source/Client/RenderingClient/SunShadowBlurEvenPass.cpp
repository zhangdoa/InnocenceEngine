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
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	auto l_shadowMapResolution = SunShadowGeometryProcessPass::Get().GetShadowMapResolution();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;

	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("SunShadowBlurEvenPass/");
	m_SPC->m_ShaderFilePaths.m_CSPath = "sunShadowBlurPassEven.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("SunShadowBlurEvenPass/");

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(3);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs = m_RPDC->m_ResourceBindingLayoutDescs;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("SunShadowBlurEvenPass/");

	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowBlurEvenPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool SunShadowBlurEvenPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

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
	auto l_perFrameGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_perFrameGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, SunShadowBlurOddPass::Get().GetResult(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, l_shadowMapResolution, l_shadowMapResolution, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, SunShadowBlurOddPass::Get().GetResult(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_TDC, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* SunShadowBlurEvenPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* SunShadowBlurEvenPass::GetResult()
{
	return m_TDC;
}
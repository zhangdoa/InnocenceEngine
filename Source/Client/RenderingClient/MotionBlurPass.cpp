#include "MotionBlurPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace MotionBlurPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
	TextureDataComponent* m_TDC;
}

bool MotionBlurPass::Setup()
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("MotionBlurPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "motionBlurPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("MotionBlurPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("MotionBlurPass/");
	m_SDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Border;
	m_SDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Border;

	m_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("MotionBlurPass/");
	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	return true;
}

bool MotionBlurPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC);

	return true;
}

bool MotionBlurPass::Render(IResourceBinder* input)
{
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, m_SDC->m_ResourceBinder, 3, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 4, 0, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 0, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, input, 1, 1);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->DispatchCompute(m_RPDC, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[3], 0, 0);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, input, 1, 1);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Compute, m_TDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool MotionBlurPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent* MotionBlurPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* MotionBlurPass::GetSPC()
{
	return m_SPC;
}

IResourceBinder* MotionBlurPass::GetResult()
{
	return m_TDC->m_ResourceBinder;
}
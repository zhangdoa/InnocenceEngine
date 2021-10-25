#include "VXGIScreenSpaceFeedbackPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VXGIRenderer.h"
#include "OpaquePass.h"
#include "LightPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool VXGIScreenSpaceFeedbackPass::Setup(ISystemConfig *systemConfig)
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("VoxelScreenSpaceFeedbackPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "voxelScreenSpaceFeedBackPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("VoxelScreenSpaceFeedbackPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("VoxelScreenSpaceFeedbackVolume/");
	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_TDC->m_TextureDesc.Width = VXGIRenderer::Get().GetVXGIRenderingConfig().m_voxelizationResolution;
	m_TDC->m_TextureDesc.Height = VXGIRenderer::Get().GetVXGIRenderingConfig().m_voxelizationResolution;
	m_TDC->m_TextureDesc.DepthOrArraySize = VXGIRenderer::Get().GetVXGIRenderingConfig().m_voxelizationResolution;
	m_TDC->m_TextureDesc.Usage = TextureUsage::Sample;
	m_TDC->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_TDC->m_TextureDesc.UseMipMap = true;

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(5);

	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 9;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIScreenSpaceFeedbackPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC);
	
	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIScreenSpaceFeedbackPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIScreenSpaceFeedbackPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIScreenSpaceFeedbackPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingContext = reinterpret_cast<VXGIScreenSpaceFeedbackPassRenderingContext*>(renderingContext);
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();
	auto l_perFrameGBDC = DefaultGPUBuffers::GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_perFrameGBDC, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 4, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, OpaquePass::Get().GetRPDC()->m_RenderTargets[0], 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, LightPass::Get().GetIlluminanceResult(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_output, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, OpaquePass::Get().GetRPDC()->m_RenderTargets[0], 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, LightPass::Get().GetIlluminanceResult(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_output, 2, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* VXGIScreenSpaceFeedbackPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* VXGIScreenSpaceFeedbackPass::GetResult()
{
	return m_TDC;
}
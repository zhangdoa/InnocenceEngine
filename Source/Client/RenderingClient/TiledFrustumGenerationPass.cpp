#include "TiledFrustumGenerationPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool TiledFrustumGenerationPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_numThreads = TVec4<uint32_t>((uint32_t)l_numThreadsX, (uint32_t)l_numThreadsY, 1, 0);
	m_numThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_numThreads.x * m_numThreads.y;

	m_tiledFrustum = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TiledFrustumGPUBuffer/");
	m_tiledFrustum->m_GPUAccessibility = Accessibility::ReadWrite;
	m_tiledFrustum->m_ElementCount = l_elementCount;
	m_tiledFrustum->m_ElementSize = 64;

	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("TiledFrustumGenerationPass/");
	m_SPC->m_ShaderFilePaths.m_CSPath = "tileFrustum.comp/";	

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("TiledFrustumGenerationPass/");
	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(3);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 6;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_RPDC->m_ShaderProgram = m_SPC;
	
	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool TiledFrustumGenerationPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_tiledFrustum);
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TiledFrustumGenerationPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TiledFrustumGenerationPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TiledFrustumGenerationPass::PrepareCommandList(IRenderingContext* renderingContext)
{		
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	DispatchParamsConstantBuffer l_tiledFrustumWorkload;
	l_tiledFrustumWorkload.numThreadGroups = m_numThreadGroups;
	l_tiledFrustumWorkload.numThreads = m_numThreads;

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_tiledFrustumWorkload, 0, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_dispatchParamsGBDC, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_tiledFrustum, 2, Accessibility::ReadWrite, 0);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_tiledFrustum, 2, Accessibility::ReadWrite, 0);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* TiledFrustumGenerationPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* TiledFrustumGenerationPass::GetTiledFrustum()
{
	return m_tiledFrustum;
}

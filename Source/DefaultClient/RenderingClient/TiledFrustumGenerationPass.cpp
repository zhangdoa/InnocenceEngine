#include "TiledFrustumGenerationPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/GraphicsResourceService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

using namespace Inno;

bool TiledFrustumGenerationPass::Setup(IServiceConfig* systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	m_ShaderProgramComp = l_rsService->AddShaderProgramComponent("TiledFrustumGenerationPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "tileFrustum.comp/";

	m_DispatchParamsGPUBufferComp = l_rsService->AddGPUBufferComponent("TiledFrustumDispatchParams/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&TiledFrustumGenerationPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp = l_rsService->AddRenderPassComponent("TiledFrustumGenerationPass/");
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// b1 - DispatchParams
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	// u0 - TiledFrustum
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = l_rsService->AddCommandListComponent("TiledFrustumGenerationPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool TiledFrustumGenerationPass::Initialize()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	m_DispatchParamsGPUBufferComp->m_ElementCount = 1;
	m_DispatchParamsGPUBufferComp->m_ElementSize = sizeof(DispatchParamsConstantBuffer);
	m_DispatchParamsGPUBufferComp->m_GPUAccessibility = Accessibility::ReadOnly;

	l_rsService->Initialize(m_DispatchParamsGPUBufferComp);
	l_rsService->Initialize(m_ShaderProgramComp);
	l_rsService->Initialize(m_RenderPassComp);
	l_rsService->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool TiledFrustumGenerationPass::Update()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	DispatchParamsConstantBuffer l_tiledFrustumWorkload;
	l_tiledFrustumWorkload.numThreadGroups = m_numThreadGroups;
	l_tiledFrustumWorkload.numThreads = m_numThreads;

	l_rsService->Upload(m_DispatchParamsGPUBufferComp, &l_tiledFrustumWorkload, 0, 1);

	return true;
}

bool TiledFrustumGenerationPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_rsService->Delete(m_TiledFrustum);
	l_rsService->Delete(m_DispatchParamsGPUBufferComp);
	l_rsService->Delete(m_RenderPassComp);
	l_rsService->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TiledFrustumGenerationPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TiledFrustumGenerationPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_TiledFrustum->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_hwService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_hwService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
	l_hwService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Compute);
	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_DispatchParamsGPUBufferComp, 1);
	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_TiledFrustum, 2);

	l_hwService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	l_hwService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);
	
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* TiledFrustumGenerationPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* TiledFrustumGenerationPass::GetTiledFrustum()
{
	return m_TiledFrustum;
}

bool Inno::TiledFrustumGenerationPass::RenderTargetsCreationFunc()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_numThreads = TVec4<uint32_t>((uint32_t)l_numThreadsX, (uint32_t)l_numThreadsY, 1, 0);
	m_numThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_numThreads.x * m_numThreads.y;

	if (m_TiledFrustum)
		l_rsService->Delete(m_TiledFrustum);

	m_TiledFrustum = l_rsService->AddGPUBufferComponent("TiledFrustumGPUBuffer/");
	m_TiledFrustum->m_GPUAccessibility = Accessibility::ReadWrite;
	m_TiledFrustum->m_ElementCount = l_elementCount;
	m_TiledFrustum->m_ElementSize = 64; // 4 planes to make a frustum, float3 normal + float distance for each plane

	l_rsService->Initialize(m_TiledFrustum);

	return true;
}

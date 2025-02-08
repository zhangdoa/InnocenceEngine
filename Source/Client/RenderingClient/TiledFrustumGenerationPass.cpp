#include "TiledFrustumGenerationPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool TiledFrustumGenerationPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("TiledFrustumGenerationPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "tileFrustum.comp/";

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&TiledFrustumGenerationPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("TiledFrustumGenerationPass/");
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

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool TiledFrustumGenerationPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool TiledFrustumGenerationPass::Update()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);

	DispatchParamsConstantBuffer l_tiledFrustumWorkload;
	l_tiledFrustumWorkload.numThreadGroups = m_numThreadGroups;
	l_tiledFrustumWorkload.numThreads = m_numThreads;

	l_renderingServer->Upload(l_dispatchParamsGPUBufferComp, &l_tiledFrustumWorkload, 0, 1);

	return true;
}

bool TiledFrustumGenerationPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_RenderPassComp);

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

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_tiledFrustum, 2);

	l_renderingServer->Dispatch(m_RenderPassComp, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	l_renderingServer->CommandListEnd(m_RenderPassComp);
	
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* TiledFrustumGenerationPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* TiledFrustumGenerationPass::GetTiledFrustum()
{
	return m_tiledFrustum;
}

bool Inno::TiledFrustumGenerationPass::CreateResources()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_numThreads = TVec4<uint32_t>((uint32_t)l_numThreadsX, (uint32_t)l_numThreadsY, 1, 0);
	m_numThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_numThreads.x * m_numThreads.y;

	if (m_tiledFrustum)
		l_renderingServer->Delete(m_tiledFrustum);

	m_tiledFrustum = l_renderingServer->AddGPUBufferComponent("TiledFrustumGPUBuffer/");
	m_tiledFrustum->m_GPUAccessibility = Accessibility::ReadWrite;
	m_tiledFrustum->m_ElementCount = l_elementCount;
	m_tiledFrustum->m_ElementSize = 64; // 4 planes to make a frustum, float3 normal + float distance for each plane

	return true;
}

bool Inno::TiledFrustumGenerationPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	CreateResources();

	l_renderingServer->Initialize(m_tiledFrustum);

	return true;
}

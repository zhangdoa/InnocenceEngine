#include "TiledFrustumGenerationPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

using namespace Inno;

namespace
{
	// Graph mode records bind + the two-level tiled dispatch via the TiledFrustum
	// kernel. The DispatchParams cbuffer and the resize-sized frustum buffer have
	// init/resize lifecycles the graph does not model, so they stay imperatively
	// created (SetupOwnedResources + RenderTargetsCreationFunc) and imported by
	// name; the graph never allocates them. The imperative path is the fallback.
	constexpr bool g_UseRenderGraph = true;
}

bool TiledFrustumGenerationPass::SetupFromRenderGraph()
{
	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("TiledFrustumGenerationPass");
	if (!l_node)
	{
		Log(Error, "TiledFrustumGenerationPass: render graph has no TiledFrustumGenerationPass node.");
		return false;
	}

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	// The graph imports the frustum buffer rather than allocating it: re-install
	// the pass's RT-init-func + resizable flag on the graph node's RenderPass so
	// the buffer is created at Initialize and resized through PostResize exactly as
	// the imperative path did.
	m_RenderPassComp->m_RenderPassDesc.m_UseOutputMerger = false;
	m_RenderPassComp->m_RenderPassDesc.m_Resizable = true;
	m_RenderPassComp->m_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&TiledFrustumGenerationPass::RenderTargetsCreationFunc, this);

	if (!SetupOwnedResources())
		return false;

	m_TiledFrustum = nullptr;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool TiledFrustumGenerationPass::Setup(IServiceConfig* systemConfig)
{
	if (g_UseRenderGraph)
		return SetupFromRenderGraph();

	return SetupImperative();
}

bool TiledFrustumGenerationPass::SetupOwnedResources()
{
	m_DispatchParamsGPUBufferComp = g_Engine->Get<GPUBufferResourceService>()->Add("TiledFrustumDispatchParams");
	return true;
}

bool TiledFrustumGenerationPass::SetupImperative()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("TiledFrustumGenerationPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "tileFrustum.comp";

	if (!SetupOwnedResources())
		return false;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&TiledFrustumGenerationPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("TiledFrustumGenerationPass");
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

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("TiledFrustumGenerationPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool TiledFrustumGenerationPass::Initialize()
{
	m_DispatchParamsGPUBufferComp->m_ElementCount = 1;
	m_DispatchParamsGPUBufferComp->m_ElementSize = sizeof(DispatchParamsConstantBuffer);
	m_DispatchParamsGPUBufferComp->m_GPUAccessibility = Accessibility::ReadOnly;

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_DispatchParamsGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool TiledFrustumGenerationPass::Update()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	DispatchParamsConstantBuffer l_tiledFrustumWorkload;
	l_tiledFrustumWorkload.numThreadGroups = m_numThreadGroups;
	l_tiledFrustumWorkload.numThreads = m_numThreads;

	g_Engine->Get<GPUBufferResourceService>()->Upload(m_DispatchParamsGPUBufferComp, &l_tiledFrustumWorkload, 0, 1);

	return true;
}

bool TiledFrustumGenerationPass::Terminate()
{
	g_Engine->Get<GPUBufferResourceService>()->Delete(m_TiledFrustum);
	g_Engine->Get<GPUBufferResourceService>()->Delete(m_DispatchParamsGPUBufferComp);

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
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (g_UseRenderGraph)
	{
		if (!m_TiledFrustum || m_TiledFrustum->m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("TiledFrustumGenerationPass");
		if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
			return false;

		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	if (m_TiledFrustum->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
	l_fmService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Compute);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_DispatchParamsGPUBufferComp, 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_TiledFrustum, 2);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);
	
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
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_numThreads = TVec4<uint32_t>((uint32_t)l_numThreadsX, (uint32_t)l_numThreadsY, 1, 0);
	m_numThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_numThreads.x * m_numThreads.y;

	if (m_TiledFrustum)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_TiledFrustum);

	m_TiledFrustum = g_Engine->Get<GPUBufferResourceService>()->Add("TiledFrustumGPUBuffer");
	m_TiledFrustum->m_GPUAccessibility = Accessibility::ReadWrite;
	m_TiledFrustum->m_ElementCount = l_elementCount;
	m_TiledFrustum->m_ElementSize = 64; // 4 planes to make a frustum, float3 normal + float distance for each plane

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_TiledFrustum);

	return true;
}

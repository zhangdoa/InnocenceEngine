#include "PTHashGridCachePurgeTilesPass.h"

#include "PTPass.h"
#include "HashGridCacheConstants.h"

#include "../../Engine/Component/GPUBufferComponent.h"
#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool PTHashGridCachePurgeTilesPass::Setup(IServiceConfig* systemConfig)
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
	{
		// Toggle off: pass stays Terminated, never advertises Activated to
		// the dispatcher, never allocates a shader program or render pass.
		// Mirrors the PTPass / UpdateTiles `if constexpr` gate so
		// the cache-off build leaves no UAV transitions, no command-list
		// recording, and no DXIL referencing this shader.
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("PTHashGridCachePurgeTilesPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "PTHashGridCachePurgeTiles.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PTHashGridCachePurgeTilesPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger  = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// 1 CB + 2 UAVs: the FrameCount CB feeds the decay-difference compare,
	// HashBuffer is the eviction target (zeroed on free), DecayTileBuffer
	// is read for the marker and zeroed on free for symmetry with the
	// scene-load clear path.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);

	// b0 - FrameCountCB
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex   = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage       = ShaderStage::Compute;

	// u0 - HashBuffer (read & zero-on-free)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex        = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage            = ShaderStage::Compute;

	// u1 - DecayTileBuffer (read marker, zero on free)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex        = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage            = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PTHashGridCachePurgeTilesPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("PTHashGridCachePurgeTilesPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PTHashGridCachePurgeTilesPass::Initialize()
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
		return true;

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool PTHashGridCachePurgeTilesPass::Update()
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
		return true;

	// Activation gates on the PTPass cache buffers + FrameCount
	// CB being allocated and Activated. The owner pass drives the toggle
	// downstream, so once the path tracer goes Activated, this pass does
	// too on the next Update.
	auto* l_owner    = &PTPass::Get();
	auto* l_frameCB  = l_owner->GetFrameCountCB();
	auto* l_hash     = l_owner->GetHashGridCacheHashBuffer();
	auto* l_decay    = l_owner->GetHashGridCacheDecayTileBuffer();

	const bool l_buffersReady =
		l_frameCB && l_frameCB->m_ObjectStatus == ObjectStatus::Activated &&
		l_hash    && l_hash->m_ObjectStatus    == ObjectStatus::Activated &&
		l_decay   && l_decay->m_ObjectStatus   == ObjectStatus::Activated;

	m_ObjectStatus = l_buffersReady ? ObjectStatus::Activated : ObjectStatus::Suspended;
	return true;
}

bool PTHashGridCachePurgeTilesPass::Terminate()
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
	{
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus PTHashGridCachePurgeTilesPass::GetStatus()
{
	return m_ObjectStatus;
}

bool PTHashGridCachePurgeTilesPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
		return true;

	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto* l_owner   = &PTPass::Get();
	auto* l_frameCB = l_owner->GetFrameCountCB();
	auto* l_hash    = l_owner->GetHashGridCacheHashBuffer();
	auto* l_decay   = l_owner->GetHashGridCacheDecayTileBuffer();

	if (!l_frameCB || !l_hash || !l_decay)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_frameCB, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_hash,    1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_decay,   2);

	// One thread group per tile slot: NUM_BUCKETS * NUM_TILES_PER_BUCKET
	// groups, each running 64 threads of which only thread 0 acts. The
	// 64-thread shape mirrors UpdateTiles so the two passes share dispatch
	// arithmetic; the wasted threads idle in-wave at no extra cost. An
	// unsaturated cache costs one uint load per group (the HashBuffer
	// early-out in the shader skips slots whose tile_hash is zero).
	using namespace Inno::PTHashGridCache;
	const uint32_t l_groupCount = NUM_TILES;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groupCount, 1u, 1u);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

RenderPassComponent* PTHashGridCachePurgeTilesPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

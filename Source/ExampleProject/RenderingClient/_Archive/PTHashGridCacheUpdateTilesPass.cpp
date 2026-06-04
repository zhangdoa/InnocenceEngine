#include "PTHashGridCacheUpdateTilesPass.h"

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

bool PTHashGridCacheUpdateTilesPass::Setup(IServiceConfig* systemConfig)
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
	{
		// Toggle off: pass stays Terminated, never advertises Activated to
		// the dispatcher, never allocates a shader program or render pass.
		// Mirrors the PTPass `if constexpr` gate so the cache-off
		// build leaves no UAV transitions, no command-list recording, and
		// no DXIL referencing this shader.
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("PTHashGridCacheUpdateTilesPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "PTHashGridCacheUpdateTiles.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PTHashGridCacheUpdateTilesPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger  = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// 1 CB + 5 UAVs (HashBuffer + direct-pair + indirect-pair). HashBuffer
	// is bound writable to match its layout declaration in
	// PTHashGridCache.hlsl (InsertCell uses InterlockedCompareExchange on
	// it elsewhere); this kernel only reads it, but the binding must
	// agree across every shader sharing the UAV.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(6);

	// b0 - HashGridCacheCB
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex   = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage       = ShaderStage::Compute;

	// u0 - HashBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex        = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage            = ShaderStage::Compute;

	// u1 - UpdateCellValueBuffer (direct-lobe atomic-sum scratch — read & cleared)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex        = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage            = ShaderStage::Compute;

	// u2 - ValueBuffer (direct-lobe persistent running-mean uint2)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex        = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage            = ShaderStage::Compute;

	// u3 - UpdateCellValueIndirectBuffer (indirect-lobe atomic-sum scratch — read & cleared)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex        = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage            = ShaderStage::Compute;

	// u4 - ValueIndirectBuffer (indirect-lobe persistent running-mean uint2)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex        = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage            = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PTHashGridCacheUpdateTilesPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("PTHashGridCacheUpdateTilesPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PTHashGridCacheUpdateTilesPass::Initialize()
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

bool PTHashGridCacheUpdateTilesPass::Update()
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
		return true;

	// Activation is gated on the PTPass cache buffers being
	// allocated and Activated. The owner pass also drives the toggle
	// downstream, so once the path tracer goes Activated, this pass does
	// too on the next Update.
	auto* l_owner = &PTPass::Get();
	auto* l_cb              = l_owner->GetHashGridCacheCB();
	auto* l_hash            = l_owner->GetHashGridCacheHashBuffer();
	auto* l_scratch         = l_owner->GetHashGridCacheUpdateCellValueBuffer();
	auto* l_value           = l_owner->GetHashGridCacheValueBuffer();
	auto* l_scratchIndirect = l_owner->GetHashGridCacheUpdateCellValueIndirectBuffer();
	auto* l_valueIndirect   = l_owner->GetHashGridCacheValueIndirectBuffer();

	const bool l_buffersReady =
		l_cb              && l_cb->m_ObjectStatus              == ObjectStatus::Activated &&
		l_hash            && l_hash->m_ObjectStatus            == ObjectStatus::Activated &&
		l_scratch         && l_scratch->m_ObjectStatus         == ObjectStatus::Activated &&
		l_value           && l_value->m_ObjectStatus           == ObjectStatus::Activated &&
		l_scratchIndirect && l_scratchIndirect->m_ObjectStatus == ObjectStatus::Activated &&
		l_valueIndirect   && l_valueIndirect->m_ObjectStatus   == ObjectStatus::Activated;

	m_ObjectStatus = l_buffersReady ? ObjectStatus::Activated : ObjectStatus::Suspended;
	return true;
}

bool PTHashGridCacheUpdateTilesPass::Terminate()
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

ObjectStatus PTHashGridCacheUpdateTilesPass::GetStatus()
{
	return m_ObjectStatus;
}

bool PTHashGridCacheUpdateTilesPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
		return true;

	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto* l_owner = &PTPass::Get();
	auto* l_cb              = l_owner->GetHashGridCacheCB();
	auto* l_hash            = l_owner->GetHashGridCacheHashBuffer();
	auto* l_scratch         = l_owner->GetHashGridCacheUpdateCellValueBuffer();
	auto* l_value           = l_owner->GetHashGridCacheValueBuffer();
	auto* l_scratchIndirect = l_owner->GetHashGridCacheUpdateCellValueIndirectBuffer();
	auto* l_valueIndirect   = l_owner->GetHashGridCacheValueIndirectBuffer();

	if (!l_cb || !l_hash || !l_scratch || !l_value || !l_scratchIndirect || !l_valueIndirect)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_cb,              0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_hash,            1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_scratch,         2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_value,           3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_scratchIndirect, 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_valueIndirect,   5);

	// One thread group per tile: 64 threads (one per mip-0 cell at the
	// engine's TILE_CELL_RATIO == 8). Total tile slot count = NUM_BUCKETS *
	// NUM_TILES_PER_BUCKET (~131K at the configured sizing); unclaimed
	// tiles early-out on HashBuffer == 0 inside the kernel, so the cost on
	// an empty cache is one uint load per thread. No dirty-tile dispatch
	// list (Capsaicin's UpdateTileBuffer / IndirectDispatch) — until
	// PurgeTiles + dirty-list tracking land, the wide dispatch is the
	// simplest correct fixed-cost shape.
	using namespace Inno::PTHashGridCache;
	const uint32_t l_groupCount = NUM_TILES;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groupCount, 1u, 1u);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

RenderPassComponent* PTHashGridCacheUpdateTilesPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

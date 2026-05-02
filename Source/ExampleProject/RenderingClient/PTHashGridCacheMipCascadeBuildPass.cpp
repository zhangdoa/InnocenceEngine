#include "PTHashGridCacheMipCascadeBuildPass.h"

#include "GPUPathTracerPass.h"
#include "HashGridCacheConstants.h"

#include "../../Engine/Component/GPUBufferComponent.h"
#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool PTHashGridCacheMipCascadeBuildPass::Setup(IServiceConfig* systemConfig)
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
	{
		// Toggle off: pass stays Terminated, never advertises Activated to
		// the dispatcher, never allocates a shader program or render pass.
		// Mirrors the GPUPathTracerPass / UpdateTiles / PurgeTiles
		// `if constexpr` gate so the cache-off build leaves no UAV
		// transitions, no command-list recording, and no DXIL referencing
		// this shader.
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("PTHashGridCacheMipCascadeBuildPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "PTHashGridCacheMipCascadeBuild.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PTHashGridCacheMipCascadeBuildPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger  = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// 1 CB + 3 UAVs (D1-reversal CL B: was 1 CB + 2 UAVs; the indirect-lobe
	// ValueIndirectBuffer added at u2, mirroring the direct ValueBuffer
	// at u1 one-for-one). HashBuffer is read-only here (the early-out
	// predicate); both ValueBuffers are read at mip 0 and written at mips
	// 1-3. Bound writable to match the layout declaration shared with
	// UpdateTiles / the path tracer raygen — every shader sharing the
	// buffer must agree on the binding access, even if a given pass only
	// reads one direction. UpdateCellValue scratch is NOT used by the
	// cascade build (its consumer is UpdateTiles' running-mean merge,
	// which has already run by the time MipCascadeBuild starts), so only
	// the persistent Value buffer pair grows the binding list.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(4);

	// b0 - HashGridCacheCB
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex   = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage       = ShaderStage::Compute;

	// u0 - HashBuffer (read-only — the empty-tile early-out)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex        = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage            = ShaderStage::Compute;

	// u1 - ValueBuffer (direct lobe — read at mip 0, write at mips 1-3 of every claimed tile)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex        = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage            = ShaderStage::Compute;

	// u2 - ValueIndirectBuffer (indirect lobe — same access shape as u1)
	// D1-reversal CL B: cascade twin for the indirect lobe. Until CL C
	// lights up the indirect scratch writers, mip-0 reads come back as
	// zero so the cascade aggregation produces zero parents — runtime
	// no-op, structural delta only.
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex      = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex        = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage            = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PTHashGridCacheMipCascadeBuildPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("PTHashGridCacheMipCascadeBuildPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PTHashGridCacheMipCascadeBuildPass::Initialize()
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

bool PTHashGridCacheMipCascadeBuildPass::Update()
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
		return true;

	// Activation gates on the GPUPathTracerPass cache buffers being
	// allocated and Activated. The owner pass drives the toggle downstream,
	// so once the path tracer goes Activated, this pass does too on the
	// next Update.
	auto* l_owner = &GPUPathTracerPass::Get();
	auto* l_cb            = l_owner->GetHashGridCacheCB();
	auto* l_hash          = l_owner->GetHashGridCacheHashBuffer();
	auto* l_value         = l_owner->GetHashGridCacheValueBuffer();
	auto* l_valueIndirect = l_owner->GetHashGridCacheValueIndirectBuffer();

	const bool l_buffersReady =
		l_cb            && l_cb->m_ObjectStatus            == ObjectStatus::Activated &&
		l_hash          && l_hash->m_ObjectStatus          == ObjectStatus::Activated &&
		l_value         && l_value->m_ObjectStatus         == ObjectStatus::Activated &&
		l_valueIndirect && l_valueIndirect->m_ObjectStatus == ObjectStatus::Activated;

	m_ObjectStatus = l_buffersReady ? ObjectStatus::Activated : ObjectStatus::Suspended;
	return true;
}

bool PTHashGridCacheMipCascadeBuildPass::Terminate()
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

ObjectStatus PTHashGridCacheMipCascadeBuildPass::GetStatus()
{
	return m_ObjectStatus;
}

bool PTHashGridCacheMipCascadeBuildPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if constexpr (!Inno::PTHashGridCache::ENABLED)
		return true;

	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto* l_owner = &GPUPathTracerPass::Get();
	auto* l_cb            = l_owner->GetHashGridCacheCB();
	auto* l_hash          = l_owner->GetHashGridCacheHashBuffer();
	auto* l_value         = l_owner->GetHashGridCacheValueBuffer();
	auto* l_valueIndirect = l_owner->GetHashGridCacheValueIndirectBuffer();

	if (!l_cb || !l_hash || !l_value || !l_valueIndirect)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_cb,            0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_hash,          1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_value,         2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_valueIndirect, 3);

	// One thread group per tile slot: NUM_BUCKETS * NUM_TILES_PER_BUCKET
	// groups, each running 64 threads of which the mip-1 stage uses 16, the
	// mip-2 stage uses 4, and the mip-3 stage uses 1. The 64-thread shape
	// mirrors UpdateTiles / PurgeTiles so the three cache passes share
	// dispatch arithmetic; the over-subscribed threads idle in-wave at no
	// extra cost. An unsaturated cache costs one uint load per group (the
	// HashBuffer early-out in the shader skips slots whose tile_hash is
	// zero, identical shape to the sibling passes).
	using namespace Inno::PTHashGridCache;
	const uint32_t l_groupCount = NUM_TILES;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groupCount, 1u, 1u);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

RenderPassComponent* PTHashGridCacheMipCascadeBuildPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

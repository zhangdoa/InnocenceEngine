#include "GPUPathTracerPass.h"
#include "HashGridCacheConstants.h"

#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_RayTracingSPC);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RayTracingRenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_MaterialSampler);



	CreateAccumulationBuffer();

	// FrameCountCB: single uint32
	m_FrameCountCB = g_Engine->Get<GPUBufferResourceService>()->Add("GPUPathTracerFrameCountCB");
	m_FrameCountCB->m_ElementCount      = 1;
	m_FrameCountCB->m_ElementSize       = sizeof(uint32_t);
	m_FrameCountCB->m_CPUAccessibility  = Accessibility::WriteOnly;
	m_FrameCountCB->m_GPUAccessibility  = Accessibility::ReadOnly;
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_FrameCountCB);

	// LightCountCB: two uint32 (point count, sphere count) + two padding uint32
	m_LightCountCB = g_Engine->Get<GPUBufferResourceService>()->Add("GPUPathTracerLightCountCB");
	m_LightCountCB->m_ElementCount      = 1;
	m_LightCountCB->m_ElementSize       = sizeof(PathTracerLightCountData);
	m_LightCountCB->m_CPUAccessibility  = Accessibility::WriteOnly;
	m_LightCountCB->m_GPUAccessibility  = Accessibility::ReadOnly;
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_LightCountCB);

	if constexpr (Inno::PTHashGridCache::ENABLED)
	{
		auto l_bufService = g_Engine->Get<GPUBufferResourceService>();
		using namespace Inno::PTHashGridCache;

		m_HashGridCacheCB = l_bufService->Add("PTHashGridCacheCB");
		m_HashGridCacheCB->m_ElementCount     = 1;
		m_HashGridCacheCB->m_ElementSize      = sizeof(HashGridCacheConstants);
		m_HashGridCacheCB->m_CPUAccessibility = Accessibility::WriteOnly;
		m_HashGridCacheCB->m_GPUAccessibility = Accessibility::ReadOnly;
		l_bufService->Initialize(m_HashGridCacheCB);

		m_HashGridCache_HashBuffer = l_bufService->Add("PTHashGridCache_HashBuffer");
		m_HashGridCache_HashBuffer->m_ElementCount     = NUM_TILES;
		m_HashGridCache_HashBuffer->m_ElementSize      = sizeof(uint32_t);
		m_HashGridCache_HashBuffer->m_CPUAccessibility = Accessibility::Immutable;
		m_HashGridCache_HashBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		l_bufService->Initialize(m_HashGridCache_HashBuffer);

		m_HashGridCache_DecayTileBuffer = l_bufService->Add("PTHashGridCache_DecayTileBuffer");
		m_HashGridCache_DecayTileBuffer->m_ElementCount     = NUM_TILES;
		m_HashGridCache_DecayTileBuffer->m_ElementSize      = sizeof(uint32_t);
		m_HashGridCache_DecayTileBuffer->m_CPUAccessibility = Accessibility::Immutable;
		m_HashGridCache_DecayTileBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		l_bufService->Initialize(m_HashGridCache_DecayTileBuffer);

		// 4 × uint per cell: rgb sums + sample count, atomic-add scratch.
		m_HashGridCache_UpdateCellValueBuffer = l_bufService->Add("PTHashGridCache_UpdateCellValueBuffer");
		m_HashGridCache_UpdateCellValueBuffer->m_ElementCount     = NUM_CELLS * 4u;
		m_HashGridCache_UpdateCellValueBuffer->m_ElementSize      = sizeof(uint32_t);
		m_HashGridCache_UpdateCellValueBuffer->m_CPUAccessibility = Accessibility::Immutable;
		m_HashGridCache_UpdateCellValueBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		l_bufService->Initialize(m_HashGridCache_UpdateCellValueBuffer);

		// uint2 per cell: packHalf4 of (radiance_total.rgb, sample_count).
		// Reserved for the read-site CL; first CL leaves zero.
		m_HashGridCache_ValueBuffer = l_bufService->Add("PTHashGridCache_ValueBuffer");
		m_HashGridCache_ValueBuffer->m_ElementCount     = NUM_CELLS;
		m_HashGridCache_ValueBuffer->m_ElementSize      = sizeof(uint32_t) * 2u;
		m_HashGridCache_ValueBuffer->m_CPUAccessibility = Accessibility::Immutable;
		m_HashGridCache_ValueBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		l_bufService->Initialize(m_HashGridCache_ValueBuffer);

		// D1-reversal chain CL A — indirect-mirror pair. Allocation shape
		// mirrors the direct pair above (Capsaicin gi1.cpp:497-553 — the
		// `gi1_use_multibounce` branch creates ValueIndirectBuffer as
		// uint2[num_cells] and UpdateCellValueIndirectBuffer as uint[num_cells*4]
		// alongside the unconditional direct pair). This CL allocates and
		// clears the buffers; they are dead data — no shader binding, no
		// dispatch reads or writes. The integrator + UpdateTiles wiring
		// lands in subsequent CLs of the chain.
		m_HashGridCache_UpdateCellValueIndirectBuffer = l_bufService->Add("PTHashGridCache_UpdateCellValueIndirectBuffer");
		m_HashGridCache_UpdateCellValueIndirectBuffer->m_ElementCount     = NUM_CELLS * 4u;
		m_HashGridCache_UpdateCellValueIndirectBuffer->m_ElementSize      = sizeof(uint32_t);
		m_HashGridCache_UpdateCellValueIndirectBuffer->m_CPUAccessibility = Accessibility::Immutable;
		m_HashGridCache_UpdateCellValueIndirectBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		l_bufService->Initialize(m_HashGridCache_UpdateCellValueIndirectBuffer);

		m_HashGridCache_ValueIndirectBuffer = l_bufService->Add("PTHashGridCache_ValueIndirectBuffer");
		m_HashGridCache_ValueIndirectBuffer->m_ElementCount     = NUM_CELLS;
		m_HashGridCache_ValueIndirectBuffer->m_ElementSize      = sizeof(uint32_t) * 2u;
		m_HashGridCache_ValueIndirectBuffer->m_CPUAccessibility = Accessibility::Immutable;
		m_HashGridCache_ValueIndirectBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		l_bufService->Initialize(m_HashGridCache_ValueIndirectBuffer);

		// Scene-load is the natural reset boundary; queue a clear for the
		// first PrepareCommandList that runs.
		m_HashGridCachePendingClear = true;
	}

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

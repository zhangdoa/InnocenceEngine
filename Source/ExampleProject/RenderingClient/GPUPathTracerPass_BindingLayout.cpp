#include "GPUPathTracerPass.h"
#include "HashGridCacheConstants.h"
#include "PTDenoiseConstants.h"

using namespace Inno;

void GPUPathTracerPass::ConfigureRaytracingBindings()
{
	// Raygen binding-count invariant. The HLSL register declarations in
	// GPUPathTracerRayGen.hlsl (cache block: b3 + u1..u6 inside the
	// PT_HASH_GRID_CACHE_ENABLED branch; denoiser block: b4 + u7..u10
	// inside the PT_DENOISE_ENABLED branch) and the matching descriptor
	// entries below must move in lockstep — a count drift on either side
	// produces a silent root-signature / DXIL mismatch (b9a103cc PSO-failure
	// precedent). The sibling cache passes hold their own counts:
	// PTHashGridCacheUpdateTilesPass = 1 CB + 5 UAVs,
	// PTHashGridCacheMipCascadeBuildPass = 1 CB + 3 UAVs,
	// PTHashGridCachePurgeTilesPass = 1 CB + 2 UAVs.
	constexpr size_t l_baseBindingCount    = 12;
	constexpr size_t l_cacheBindingCount   = Inno::PTHashGridCache::ENABLED ? 7 : 0;
	constexpr size_t l_denoiseBindingCount = Inno::PTDenoise::ENABLED       ? 7 : 0;
	static_assert(!Inno::PTHashGridCache::ENABLED || l_cacheBindingCount == 7,
		"GPUPathTracer raygen cache-binding count must be 7 (b3 + u1..u6). "
		"u1=HashBuffer, u2=DecayTileBuffer, u3=UpdateCellValueBuffer (direct "
		"scratch), u4=ValueBuffer (direct persistent), u5=UpdateCellValueIndirectBuffer "
		"(indirect scratch — Capsaicin gi1.comp:1948-1989 UpdateMultibounceCells "
		"target), u6=ValueIndirectBuffer (indirect persistent — Site-3 read at "
		"GPUPathTracerRayGen.hlsl combines per-lobe means). The count is "
		"toggle-gated: when PTHashGridCache::ENABLED is false the assertion "
		"short-circuits and the cache descriptors are not allocated. Drift "
		"on either the HLSL register decls or the layout block below is a "
		"PSO-create failure surface (b9a103cc precedent).");
	static_assert(!Inno::PTDenoise::ENABLED || l_denoiseBindingCount == 7,
		"GPUPathTracer raygen denoiser-binding count must be 7 (b4 + u7..u12). "
		"b4=PerFrameConstantBufferPrev (engine ping-pong CB carrying the "
		"previous frame's view + p_original for primary-hit motion-vector "
		"reprojection), u7=PT-GBuffer Position+InstanceID, u8=Normal+Metalness, "
		"u9=Albedo+Roughness, u10=MotionVec+HitDist, u11=RadianceDiffuse, "
		"u12=RadianceSpecular. The two radiance UAVs (CL-2) are owned by "
		"PTDenoiseTemporalPass; the path tracer borrows them by accessor so "
		"the integrator can write the SVGF demodulated diffuse / specular "
		"channels at the AccumBuffer-composition site. Channel layout per "
		"common/PTDenoiseShared.hlsl, mirroring the rasterizer GBuffer so "
		"DecodeGBuffer (common/lightPassCommon.hlsl) reads them unchanged. "
		"Toggle-gated like the cache block above: when PTDenoise::ENABLED is "
		"false the assertion short-circuits and the denoiser descriptors are "
		"not allocated. Two toggles are independent — both can be on, off, or "
		"either-on with no shared resources. Drift on the HLSL register decls "
		"or the layout block below is a PSO-create failure surface.");
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs.resize(
		l_baseBindingCount + l_cacheBindingCount + l_denoiseBindingCount);

	// b0 - PerFrameCB (set 0, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex   = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage       = m_ShaderStage;

	// b1 - FrameCountCB (set 0, binding 1)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex   = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage       = m_ShaderStage;

	// t0 - TLAS (set 1, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUBufferUsage         = GPUBufferUsage::TLAS;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage            = m_ShaderStage;

	// t1 - MaterialBuffer (set 1, binding 1, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex        = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage            = m_ShaderStage;

	// t8 - bindless per-mesh vertex SRV array
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUBufferUsage         = GPUBufferUsage::BindlessMeshVertex;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex        = 8;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage            = m_ShaderStage;

	// t9 - bindless per-mesh index SRV array
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUBufferUsage         = GPUBufferUsage::BindlessMeshIndex;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex        = 9;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage            = m_ShaderStage;

	// u0 - AccumulationBuffer (set 2, binding 0, ReadWrite UAV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType        = GPUResourceType::Image;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex      = 2;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage           = TextureUsage::ComputeOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility   = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage            = m_ShaderStage;

	// b2 - LightCountCB (set 0, binding 2)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType   = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex   = 2;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage       = m_ShaderStage;

	// t5 - PointLightBuffer (set 1, binding 5, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex        = 5;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[8].m_ShaderStage            = m_ShaderStage;

	// t6 - SphereLightBuffer (set 1, binding 6, SRV)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType        = GPUResourceType::Buffer;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex        = 6;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_BindingAccessibility   = Accessibility::ReadOnly;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_ResourceAccessibility  = Accessibility::ReadWrite;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[9].m_ShaderStage            = m_ShaderStage;

	// t7 - bindless material textures (set 1, binding 7, unbounded Texture2D array).
	// Engine auto-populates this descriptor table from the read-only texture heap;
	// the closest-hit shader indexes it with MaterialCB::TextureIndices (same index
	// space as the rasterizer's OpaquePass at register t3).
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType        = GPUResourceType::Image;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_TextureUsage           = TextureUsage::Sample;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex      = 1;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex        = 7;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[10].m_ShaderStage            = m_ShaderStage;

	// s0 - material sampler (set 3, binding 0)
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType        = GPUResourceType::Sampler;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex      = 3;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex        = 0;
	m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[11].m_ShaderStage            = m_ShaderStage;

	if constexpr (Inno::PTHashGridCache::ENABLED)
	{
		// b3 - HashGridCacheCB (set 0, binding 3)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_GPUResourceType   = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorSetIndex = 0;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorIndex   = 3;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[12].m_ShaderStage       = m_ShaderStage;

		// u1 - HashBuffer (set 2, binding 1, ReadWrite UAV)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorIndex        = 1;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[13].m_ShaderStage            = m_ShaderStage;

		// u2 - DecayTileBuffer (set 2, binding 2, ReadWrite UAV)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorIndex        = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[14].m_ShaderStage            = m_ShaderStage;

		// u3 - UpdateCellValueBuffer (set 2, binding 3, ReadWrite UAV)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorIndex        = 3;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[15].m_ShaderStage            = m_ShaderStage;

		// u4 - ValueBuffer (set 2, binding 4, ReadWrite UAV) — Site-3 read
		// target. Sources its values from PTHashGridCacheUpdateTilesPass'
		// running-mean resolve of UpdateCellValueBuffer.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorIndex        = 4;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_ShaderStage            = m_ShaderStage;

		// u5 - UpdateCellValueIndirectBuffer (set 2, binding 5, ReadWrite UAV) —
		// indirect-scratch target for the integrator's (b) secondary-bounce
		// write. Capsaicin gi1.comp:1948-1989 (UpdateMultibounceCells) is the
		// canonical paper-port site.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorIndex        = 5;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_ShaderStage            = m_ShaderStage;

		// u6 - ValueIndirectBuffer (set 2, binding 6, ReadWrite UAV) —
		// indirect-lobe persistent estimator. Site-3 read at
		// GPUPathTracerRayGen.hlsl reads this alongside u4 (ValueBuffer) and
		// sums the per-lobe means before the cache substitution.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorIndex        = 6;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_ShaderStage            = m_ShaderStage;
	}

	if constexpr (Inno::PTDenoise::ENABLED)
	{
		// Denoiser block follows the (optionally-allocated) cache block.
		// Index 0 of the block is `l_baseBindingCount + l_cacheBindingCount`
		// — orthogonal toggle: with cache OFF the block sits at 12..16,
		// with cache ON it sits at 19..23.
		constexpr size_t l_denoiseFirst = l_baseBindingCount + l_cacheBindingCount;

		// b4 - PerFrameConstantBufferPrev (set 0, binding 4) — engine
		// ping-pong CB carrying the previous frame's view + p_original
		// for primary-hit motion-vector reprojection. Same source the
		// rasterizer's OpaquePass.frag (b2 there) consumes; reused
		// unchanged here so the motion-vector sign convention (px_prev
		// - px_curr in pixels) matches engine-wide and GIDenoise.comp's
		// `previous_uv = uv + velocity` reprojection is reusable in CL-2.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_GPUResourceType   = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_DescriptorSetIndex = 0;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_DescriptorIndex   = 4;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_ShaderStage       = m_ShaderStage;

		// u7 - PT-GBuffer Position + InstanceID (set 2, binding 7).
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_DescriptorIndex        = 7;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_ShaderStage            = m_ShaderStage;

		// u8 - PT-GBuffer Normal + Metalness (set 2, binding 8).
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_DescriptorIndex        = 8;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_ShaderStage            = m_ShaderStage;

		// u9 - PT-GBuffer Albedo + Roughness (set 2, binding 9).
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_DescriptorIndex        = 9;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_ShaderStage            = m_ShaderStage;

		// u10 - PT-GBuffer MotionVec + HitDist (set 2, binding 10).
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_DescriptorIndex        = 10;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_ShaderStage            = m_ShaderStage;

		// u11 - PT-Denoise RadianceDiffuse (set 2, binding 11). CL-2:
		// SVGF demodulated diffuse channel — written at the AccumBuffer
		// composition site, consumed by PTDenoiseTemporalPass.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_DescriptorIndex        = 11;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_ShaderStage            = m_ShaderStage;

		// u12 - PT-Denoise RadianceSpecular (set 2, binding 12). CL-2:
		// SVGF demodulated specular channel.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_DescriptorIndex        = 12;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_ShaderStage            = m_ShaderStage;
	}
}

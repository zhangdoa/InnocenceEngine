#include "PTPass.h"
#include "HashGridCacheConstants.h"
#include "PTDenoiseConstants.h"

using namespace Inno;

void PTPass::ConfigureRaytracingBindings()
{
	// Counts MUST match the register declarations in PTRayGen.hlsl. Drift produces a
	// silent root-signature / DXIL mismatch at PSO create.
	constexpr size_t l_baseBindingCount    = 12;
	constexpr size_t l_cacheBindingCount   = Inno::PTHashGridCache::ENABLED ? 7 : 0;
	constexpr size_t l_denoiseBindingCount = Inno::PTDenoise::ENABLED       ? 7 : 0;
	static_assert(!Inno::PTHashGridCache::ENABLED || l_cacheBindingCount == 7,
		"PT raygen cache-binding count must be 7 (b3 + u1..u6).");
	static_assert(!Inno::PTDenoise::ENABLED || l_denoiseBindingCount == 7,
		"PT raygen denoiser-binding count must be 7 (b4 + u7..u12).");
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

	// t7 - bindless material textures (closest-hit indexes via MaterialCB::TextureIndices,
	// same index space as OpaquePass t3).
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

		// u4 - ValueBuffer (set 2, binding 4)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorIndex        = 4;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[16].m_ShaderStage            = m_ShaderStage;

		// u5 - UpdateCellValueIndirectBuffer (set 2, binding 5).
		// Capsaicin gi1.comp:1948-1989 (UpdateMultibounceCells).
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorIndex        = 5;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[17].m_ShaderStage            = m_ShaderStage;

		// u6 - ValueIndirectBuffer (set 2, binding 6).
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_GPUResourceType        = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorIndex        = 6;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[18].m_ShaderStage            = m_ShaderStage;
	}

	if constexpr (Inno::PTDenoise::ENABLED)
	{
		constexpr size_t l_denoiseFirst = l_baseBindingCount + l_cacheBindingCount;

		// b4 - PerFrameConstantBufferPrev (previous frame view + p_original for primary-hit
		// motion-vector reprojection). Sign convention matches OpaquePass.frag b2 engine-wide.
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_GPUResourceType   = GPUResourceType::Buffer;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_DescriptorSetIndex = 0;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_DescriptorIndex   = 4;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 0].m_ShaderStage       = m_ShaderStage;

		// u7 - PT-GBuffer Position + InstanceID
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_DescriptorIndex        = 7;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 1].m_ShaderStage            = m_ShaderStage;

		// u8 - PT-GBuffer Normal + Metalness
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_DescriptorIndex        = 8;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 2].m_ShaderStage            = m_ShaderStage;

		// u9 - PT-GBuffer Albedo + Roughness
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_DescriptorIndex        = 9;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 3].m_ShaderStage            = m_ShaderStage;

		// u10 - PT-GBuffer MotionVec + HitDist
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_DescriptorIndex        = 10;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 4].m_ShaderStage            = m_ShaderStage;

		// u11 - PT-Denoise RadianceDiffuse (per-lobe demodulated diffuse channel)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_DescriptorIndex        = 11;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 5].m_ShaderStage            = m_ShaderStage;

		// u12 - PT-Denoise RadianceSpecular (per-lobe demodulated specular channel)
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_GPUResourceType        = GPUResourceType::Image;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_DescriptorSetIndex      = 2;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_DescriptorIndex        = 12;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_TextureUsage           = TextureUsage::ComputeOnly;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_BindingAccessibility   = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_ResourceAccessibility  = Accessibility::ReadWrite;
		m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[l_denoiseFirst + 6].m_ShaderStage            = m_ShaderStage;
	}
}

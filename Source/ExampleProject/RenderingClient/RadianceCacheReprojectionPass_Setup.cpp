#include "RadianceCacheReprojectionPass.h"
#include "RadianceCacheConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"

using namespace Inno;

bool RadianceCacheReprojectionPass::Setup(IServiceConfig* systemConfig)
{

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("RadianceCacheReprojectionPass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "RadianceCacheReprojection.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("RadianceCacheReprojectionPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&RadianceCacheReprojectionPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(12);

	m_ShaderStage = ShaderStage::Compute;

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// t0 - world position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	// t1 - world normal + metallic
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = m_ShaderStage;

	// t2 - motion vector + AO + transparency
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = m_ShaderStage;

	// t3 - previous frame radiance cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = m_ShaderStage;

	// t4 - previous probe world position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = m_ShaderStage;

	// t5 - previous probe world normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage = m_ShaderStage;

	// u0 - current frame radiance cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage = m_ShaderStage;

	// u1 - probe mask (GI-1.0 §2.1.5); reprojection invalidates on sky/fail
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_ShaderStage = m_ShaderStage;

	// u2 - side cache atlas (GI-1.0 §2.1.8); mirrors atlas layout, preserved on failed reprojection
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_ShaderStage = m_ShaderStage;

	// u3 - side cache pos+frame
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ShaderStage = m_ShaderStage;

	// u4 - side cache normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_ShaderStage = m_ShaderStage;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("RadianceCacheReprojectionPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("RadianceCacheReprojectionPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool RadianceCacheReprojectionPass::RenderTargetsCreationFunc()
{

	if (m_RadianceCache_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_RadianceCache_Even);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::ComputeOnly;
	m_RadianceCache_Even = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Result (Even)");
	m_RadianceCache_Even->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_RadianceCache_Even);

	if (m_RadianceCache_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_RadianceCache_Odd);

	m_RadianceCache_Odd = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Result (Odd)");
	m_RadianceCache_Odd->m_TextureDesc = m_RadianceCache_Even->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_RadianceCache_Odd);

	if (m_ProbePosition_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Odd);

	auto l_probeTextureWidth = RadianceCache::TileCount(l_RenderPassDesc.m_RenderTargetDesc.Width);
	auto l_probeTextureHeight = RadianceCache::TileCount(l_RenderPassDesc.m_RenderTargetDesc.Height);
	m_ProbePosition_Odd = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Position (Odd)");
	m_ProbePosition_Odd->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_ProbePosition_Odd->m_TextureDesc.Width = l_probeTextureWidth;
	m_ProbePosition_Odd->m_TextureDesc.Height = l_probeTextureHeight;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbePosition_Odd);

	if (m_ProbePosition_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbePosition_Even);

	m_ProbePosition_Even = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Position (Even)");
	m_ProbePosition_Even->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbePosition_Even);

	if (m_ProbeNormal_Odd)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Odd);

	m_ProbeNormal_Odd = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Normal (Odd)");
	m_ProbeNormal_Odd->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbeNormal_Odd);

	if (m_ProbeNormal_Even)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbeNormal_Even);

	m_ProbeNormal_Even = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Normal (Even)");
	m_ProbeNormal_Even->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;

	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbeNormal_Even);

	// GI-1.0 §2.1.5 probe_mask — one uint per tile. Source of truth for
	// probe validity; single-buffered because writer (ray gen) and readers
	// (filter) live in the same frame.
	if (m_ProbeMask)
		g_Engine->Get<TextureResourceService>()->Delete(m_ProbeMask);

	m_ProbeMask = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Probe Mask");
	m_ProbeMask->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;
	m_ProbeMask->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::R;
	m_ProbeMask->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;
	g_Engine->Get<TextureResourceService>()->Initialize(m_ProbeMask);

	// GI-1.0 §2.1.8 side cache. Single-slot-per-tile "last-good" snapshot
	// of the radiance atlas + (pos, normal, frameIndex). Written on
	// successful reprojection, read when the current frame's reprojection
	// fails but the cached snapshot is still geometrically close and
	// within SIDE_CACHE_MAX_AGE frames fresh (constant lives in
	// RadianceCacheReprojection.comp). Single-buffered — the Reprojection
	// shader owns both reads and writes.
	if (m_SideCache_Atlas)
		g_Engine->Get<TextureResourceService>()->Delete(m_SideCache_Atlas);

	m_SideCache_Atlas = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Side Cache Atlas");
	m_SideCache_Atlas->m_TextureDesc = m_RadianceCache_Even->m_TextureDesc;
	g_Engine->Get<TextureResourceService>()->Initialize(m_SideCache_Atlas);

	if (m_SideCache_PosFrame)
		g_Engine->Get<TextureResourceService>()->Delete(m_SideCache_PosFrame);

	m_SideCache_PosFrame = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Side Cache PosFrame");
	m_SideCache_PosFrame->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;
	g_Engine->Get<TextureResourceService>()->Initialize(m_SideCache_PosFrame);

	if (m_SideCache_Normal)
		g_Engine->Get<TextureResourceService>()->Delete(m_SideCache_Normal);

	m_SideCache_Normal = g_Engine->Get<TextureResourceService>()->Add("Radiance Cache Side Cache Normal");
	m_SideCache_Normal->m_TextureDesc = m_ProbePosition_Odd->m_TextureDesc;
	g_Engine->Get<TextureResourceService>()->Initialize(m_SideCache_Normal);

	return true;
}

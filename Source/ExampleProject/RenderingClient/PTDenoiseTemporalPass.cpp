#include "PTDenoiseTemporalPass.h"

#include "GPUPathTracerPass.h"
#include "PTDenoiseConstants.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

namespace
{
	bool TemporalPingPongUseEven()
	{
		const uint32_t l_frame = g_Engine->Get<FrameManagementService>()->GetFrameCountSinceLaunch();
		return (l_frame % 2u) == 0u;
	}
}

bool PTDenoiseTemporalPass::Setup(IServiceConfig* systemConfig)
{
	if constexpr (!Inno::PTDenoise::ENABLED)
	{
		// Toggle off: pass stays Terminated; never advertises Activated.
		// No allocation, no shader load, no command-list creation —
		// bypass invariant matches the cache-side sibling passes.
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("PTDenoiseTemporalPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "PTDenoiseTemporal.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PTDenoiseTemporalPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType                  = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount              = 0;
	l_RenderPassDesc.m_UseOutputMerger                = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&PTDenoiseTemporalPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// 1 CB + 11 SRVs + 4 UAVs = 16 descs.
	//   b0 = PerFrame CB (current frame)
	//   t0..t4  = current-frame raygen outputs (RT0/RT1/RT3 + radianceDiffuse/specular)
	//   t5..t6  = previous-frame RT0/RT1
	//   t7..t10 = previous-frame history (radiance/moments × diffuse/specular)
	//   u0..u3  = current-frame history (radiance/moments × diffuse/specular)
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(16);

	// b0 - PerFrame CB.
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType    = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex  = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex    = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage        = ShaderStage::Compute;

	// t0..t10 — all SRV reads from compute-written textures. Mirrors the
	// PT GBuffer textures + the PTDenoiseTemporalPass-owned radiance and
	// history textures, all created with TextureUsage::ComputeOnly.
	for (uint32_t i = 0u; i < 11u; ++i)
	{
		auto& l_desc = m_RenderPassComp->m_ResourceBindingLayoutDescs[1u + i];
		l_desc.m_GPUResourceType        = GPUResourceType::Image;
		l_desc.m_DescriptorSetIndex      = 1;
		l_desc.m_DescriptorIndex        = static_cast<int32_t>(i);
		l_desc.m_TextureUsage           = TextureUsage::ComputeOnly;
		l_desc.m_BindingAccessibility   = Accessibility::ReadOnly;
		l_desc.m_ResourceAccessibility  = Accessibility::ReadWrite;
		l_desc.m_ShaderStage            = ShaderStage::Compute;
	}

	// u0..u3 — current-frame history write targets.
	for (uint32_t i = 0u; i < 4u; ++i)
	{
		auto& l_desc = m_RenderPassComp->m_ResourceBindingLayoutDescs[12u + i];
		l_desc.m_GPUResourceType        = GPUResourceType::Image;
		l_desc.m_DescriptorSetIndex      = 2;
		l_desc.m_DescriptorIndex        = static_cast<int32_t>(i);
		l_desc.m_TextureUsage           = TextureUsage::ComputeOnly;
		l_desc.m_BindingAccessibility   = Accessibility::ReadWrite;
		l_desc.m_ResourceAccessibility  = Accessibility::ReadWrite;
		l_desc.m_ShaderStage            = ShaderStage::Compute;
	}

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PTDenoiseTemporalPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("PTDenoiseTemporalPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PTDenoiseTemporalPass::Initialize()
{
	if constexpr (!Inno::PTDenoise::ENABLED)
		return true;

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool PTDenoiseTemporalPass::Update()
{
	if constexpr (!Inno::PTDenoise::ENABLED)
		return true;

	// Activation gate: history textures + the per-lobe radiance UAVs +
	// the path-tracer's GBuffer textures must all be ready. Mirrors
	// PTHashGridCacheUpdateTilesPass's owner-buffer-status check.
	auto l_owner = &GPUPathTracerPass::Get();

	auto l_isActivated = [](TextureComponent* in_Tex)
	{
		return in_Tex && in_Tex->m_ObjectStatus == ObjectStatus::Activated;
	};

	const bool l_resourcesReady =
		l_isActivated(m_RadianceDiffuse) &&
		l_isActivated(m_RadianceSpecular) &&
		l_isActivated(m_HistoryRadianceDiffuse_Even) &&
		l_isActivated(m_HistoryRadianceDiffuse_Odd) &&
		l_isActivated(m_HistoryMomentsDiffuse_Even) &&
		l_isActivated(m_HistoryMomentsDiffuse_Odd) &&
		l_isActivated(m_HistoryRadianceSpecular_Even) &&
		l_isActivated(m_HistoryRadianceSpecular_Odd) &&
		l_isActivated(m_HistoryMomentsSpecular_Even) &&
		l_isActivated(m_HistoryMomentsSpecular_Odd) &&
		l_isActivated(l_owner->GetCurrentPTGBufferPosition()) &&
		l_isActivated(l_owner->GetCurrentPTGBufferNormalMetalness()) &&
		l_isActivated(l_owner->GetCurrentPTGBufferMotionHitDist());

	m_ObjectStatus = l_resourcesReady ? ObjectStatus::Activated : ObjectStatus::Suspended;
	return true;
}

bool PTDenoiseTemporalPass::Terminate()
{
	if constexpr (!Inno::PTDenoise::ENABLED)
	{
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	auto l_texService = g_Engine->Get<TextureResourceService>();
	auto l_drop = [&](TextureComponent*& tex) { if (tex) { l_texService->Delete(tex); tex = nullptr; } };
	l_drop(m_HistoryMomentsSpecular_Odd);
	l_drop(m_HistoryMomentsSpecular_Even);
	l_drop(m_HistoryRadianceSpecular_Odd);
	l_drop(m_HistoryRadianceSpecular_Even);
	l_drop(m_HistoryMomentsDiffuse_Odd);
	l_drop(m_HistoryMomentsDiffuse_Even);
	l_drop(m_HistoryRadianceDiffuse_Odd);
	l_drop(m_HistoryRadianceDiffuse_Even);
	l_drop(m_RadianceSpecular);
	l_drop(m_RadianceDiffuse);

	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus PTDenoiseTemporalPass::GetStatus()
{
	return m_ObjectStatus;
}

RenderPassComponent* PTDenoiseTemporalPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* PTDenoiseTemporalPass::GetCurrentHistoryRadianceDiffuse()
{
	return TemporalPingPongUseEven() ? m_HistoryRadianceDiffuse_Even : m_HistoryRadianceDiffuse_Odd;
}

TextureComponent* PTDenoiseTemporalPass::GetCurrentHistoryMomentsDiffuse()
{
	return TemporalPingPongUseEven() ? m_HistoryMomentsDiffuse_Even : m_HistoryMomentsDiffuse_Odd;
}

TextureComponent* PTDenoiseTemporalPass::GetCurrentHistoryRadianceSpecular()
{
	return TemporalPingPongUseEven() ? m_HistoryRadianceSpecular_Even : m_HistoryRadianceSpecular_Odd;
}

TextureComponent* PTDenoiseTemporalPass::GetCurrentHistoryMomentsSpecular()
{
	return TemporalPingPongUseEven() ? m_HistoryMomentsSpecular_Even : m_HistoryMomentsSpecular_Odd;
}

TextureComponent* PTDenoiseTemporalPass::GetPreviousHistoryRadianceDiffuse()
{
	return TemporalPingPongUseEven() ? m_HistoryRadianceDiffuse_Odd : m_HistoryRadianceDiffuse_Even;
}

TextureComponent* PTDenoiseTemporalPass::GetPreviousHistoryMomentsDiffuse()
{
	return TemporalPingPongUseEven() ? m_HistoryMomentsDiffuse_Odd : m_HistoryMomentsDiffuse_Even;
}

TextureComponent* PTDenoiseTemporalPass::GetPreviousHistoryRadianceSpecular()
{
	return TemporalPingPongUseEven() ? m_HistoryRadianceSpecular_Odd : m_HistoryRadianceSpecular_Even;
}

TextureComponent* PTDenoiseTemporalPass::GetPreviousHistoryMomentsSpecular()
{
	return TemporalPingPongUseEven() ? m_HistoryMomentsSpecular_Odd : m_HistoryMomentsSpecular_Even;
}

#include "ExampleRenderingClient_Internal.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SunShadowRTPass.h"
#include "OpaqueCullingPass.h"
#include "OpaquePass.h"
#include "SSAOPass.h"
#include "RadianceCacheReprojectionPass.h"
#include "RadianceCacheRaytracingPass.h"
#include "RadianceCacheFilterHorizontalPass.h"
#include "RadianceCacheFilterVerticalPass.h"
#include "RadianceCacheIntegrationPass.h"
#include "GIDenoisePass.h"
#include "GIFilterHorizontalPass.h"
#include "GIFilterVerticalPass.h"
#include "TiledFrustumGenerationPass.h"
#include "LightCullingPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TAAPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "FinalBlendPass.h"
#include "GPUPathTracerPass.h"
#include "PTHashGridCachePurgeTilesPass.h"
#include "PTHashGridCacheUpdateTilesPass.h"
#include "PTHashGridCacheMipCascadeBuildPass.h"
#include "PTNRDFormatConvertPass.h"
#include "PTNRDDenoisePass.h"
#include "PTNRDCompositionPass.h"
#include "HashGridCacheConstants.h"
#include "NRDConstants.h"

#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/ViewportSourceOverride.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	// .inl included inside `namespace Inno` so its anonymous-namespace helpers stay per-TU local.
	#include "ExampleRenderingClient_Bypass.inl"

	bool ExampleRenderingClientImpl::PrepareCommands()
	{
		if (m_GPUPathTracerDesired != m_GPUPathTracerActive)
		{
			m_GPUPathTracerActive = m_GPUPathTracerDesired;
			if (m_GPUPathTracerActive)
				GPUPathTracerPass::Get().ResetAccumulation();
		}

		if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			// PurgeTiles must precede UpdateTiles so the path tracer's InsertCell can re-claim
			// the freed slots this frame; UpdateTiles must precede the path tracer so mip 0 reads
			// the freshest running means; MipCascadeBuild produces mips 1-3 (reserved for future
			// wide-footprint consumers).
			if constexpr (Inno::PTHashGridCache::ENABLED)
			{
				DispatchOrBypass(PTHashGridCachePurgeTilesPass::Get());
				DispatchOrBypass(PTHashGridCacheUpdateTilesPass::Get());
				DispatchOrBypass(PTHashGridCacheMipCascadeBuildPass::Get());
			}
			DispatchOrBypass(GPUPathTracerPass::Get());
			if constexpr (Inno::NRD::ENABLED)
			{
				DispatchOrBypass(PTNRDFormatConvertPass::Get());
				DispatchOrBypass(PTNRDDenoisePass::Get());
				DispatchOrBypass(PTNRDCompositionPass::Get());
			}
		}

		m_Canvas = FinalBlendPass::Get().GetResult();
		m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();

		if (!m_GPUPathTracerActive)
		{
			if (m_ExecuteOneShotCommands)
			{
				DispatchOrBypass(BRDFLUTPass::Get());
				DispatchOrBypass(BRDFLUTMSPass::Get());
			}

			// RT sun-shadow needs the GBuffer; runtime sequencing via WaitOnGPU on OpaquePass.
			DispatchOrBypass(SunShadowRTPass::Get());

			DispatchOrBypass(OpaqueCullingPass::Get());
			DispatchOrBypass(OpaquePass::Get());

			DispatchOrBypass(RadianceCacheReprojectionPass::Get());
			DispatchOrBypass(RadianceCacheRaytracingPass::Get());
			DispatchOrBypass(RadianceCacheFilterHorizontalPass::Get());
			DispatchOrBypass(RadianceCacheFilterVerticalPass::Get());
			DispatchOrBypass(RadianceCacheIntegrationPass::Get());
			DispatchOrBypass(GIDenoisePass::Get());
			DispatchOrBypass(GIFilterHorizontalPass::Get());
			DispatchOrBypass(GIFilterVerticalPass::Get());

			DispatchOrBypass(SSAOPass::Get());

			DispatchOrBypass(TiledFrustumGenerationPass::Get());

			DispatchOrBypass(LightCullingPass::Get());

			DispatchOrBypass(LightPass::Get());

			DispatchOrBypass(SkyPass::Get());

			DispatchOrBypass(PreTAAPass::Get());

			TAAPassRenderingContext l_TAAPassRenderingContext;
			l_TAAPassRenderingContext.m_input = PreTAAPass::Get().GetResult();
			l_TAAPassRenderingContext.m_motionVector = OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3];

			DispatchOrBypass(TAAPass::Get(), &l_TAAPassRenderingContext);
		}

		GPUResourceComponent* l_hdrSource = nullptr;
		if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			// Fall back to the raw AccumBuffer when composition is not yet activated, so the
			// visible output never goes black on a transient state (first-frame / adapter-init).
			l_hdrSource = GPUPathTracerPass::Get().GetResult();
			if constexpr (Inno::NRD::ENABLED)
			{
				if (PTNRDCompositionPass::Get().GetStatus() == ObjectStatus::Activated
					&& !IsBypassed(PTNRDCompositionPass::Get()))
				{
					l_hdrSource = PTNRDCompositionPass::Get().GetResult();
				}
			}
		}
		else
		{
			l_hdrSource = TAAPass::Get().GetResult();
		}

		if (auto l_override = ViewportSourceOverride::Get())
		{
			auto* l_pass = g_Engine->Get<RenderPassResourceService>()->Find(l_override->m_PassName.c_str());
			if (l_pass && l_pass->m_OutputMergerTarget &&
				l_override->m_RTIndex < l_pass->m_OutputMergerTarget->m_ColorOutputs.size())
			{
				auto* l_chosen = l_pass->m_OutputMergerTarget->m_ColorOutputs[l_override->m_RTIndex];
				if (l_chosen)
					l_hdrSource = l_chosen;
			}
		}

		LuminanceHistogramPassRenderingContext l_LuminanceHistogramPassRenderingContext;
		l_LuminanceHistogramPassRenderingContext.m_input = l_hdrSource;
		DispatchOrBypass(LuminanceHistogramPass::Get(), &l_LuminanceHistogramPassRenderingContext);

		DispatchOrBypass(LuminanceAveragePass::Get());

		FinalBlendPassRenderingContext l_FinalBlendPassRenderingContext;
		l_FinalBlendPassRenderingContext.m_input = l_hdrSource;
		DispatchOrBypass(FinalBlendPass::Get(), &l_FinalBlendPassRenderingContext);

		return true;
	}
}

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
#include "HashGridCacheConstants.h"
#include "NRDConstants.h"

#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/ViewportSourceOverride.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	// Bypass helpers live in a sibling .inl. Each TU that uses them includes
	// the .inl inside `namespace Inno` so the anonymous-namespace helpers stay
	// per-TU local — same pattern as the original single-TU placement, just
	// replicated per sibling now that the file is split.
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
			// PurgeTiles → UpdateTiles → MipCascadeBuild → PathTracer mirrors
			// Capsaicin gi1.cpp's PurgeTiles → ... → UpdateTiles (which fuses
			// the mip cascade in Capsaicin) collapsed for our reduced pipeline:
			// PurgeTiles frees 50-frame-stale slots so UpdateTiles' HashBuffer
			// == 0 early-out skips them and the path tracer's InsertCell can
			// re-claim them this frame. UpdateTiles resolves last frame's
			// scratch deltas into ValueBuffer / ValueIndirectBuffer at mip 0
			// so the path tracer reads the freshest running means;
			// MipCascadeBuild then aggregates 2x2 children at each level into
			// mips 1-3 of both lobes (mip 0 is what Site-3 currently reads;
			// mip 1-3 are reserved for future wide-footprint consumers). Each
			// pass is a no-op when the cache toggle is off.
			if constexpr (Inno::PTHashGridCache::ENABLED)
			{
				DispatchOrBypass(PTHashGridCachePurgeTilesPass::Get());
				DispatchOrBypass(PTHashGridCacheUpdateTilesPass::Get());
				DispatchOrBypass(PTHashGridCacheMipCascadeBuildPass::Get());
			}
			DispatchOrBypass(GPUPathTracerPass::Get());
			if constexpr (Inno::NRD::ENABLED)
			{
				// NRD format-convert runs after the path tracer
				// (TASK-77.4 CL-2). Reads the GBuffer-equivalent
				// textures + per-lobe radiance UAVs the raygen just
				// wrote, packs them into the five textures NRD
				// ReBLUR consumes (IN_VIEWZ / IN_NORMAL_ROUGHNESS /
				// IN_MV / IN_DIFF/SPEC_RADIANCE_HITDIST). CL-2 ships
				// invisibly behind the AccumBuffer write — this
				// pass's outputs are unconsumed until CL-3 wires
				// PTNRDDenoisePass + PTNRDCompositionPass.
				DispatchOrBypass(PTNRDFormatConvertPass::Get());
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

			// TASK-138: dispatch RT sun-shadow rays after the GBuffer is
			// available (PrepareCommandList only records — sequencing is
			// enforced in ExecuteCommands via WaitOnGPU on OpaquePass).
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

		// Default viewport source: PT result if active, else TAA result.
		// ViewportSourceOverride lets a tooling client substitute any
		// pass's color RT for the default.
		GPUResourceComponent* l_hdrSource = nullptr;
		if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hdrSource = GPUPathTracerPass::Get().GetResult();
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

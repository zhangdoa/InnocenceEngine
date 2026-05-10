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

#include "../../Engine/Services/DevToggleRegistry.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/FrameManagementService.h"

#include "../../Engine/Engine.h"

#include <cstdlib>
#include <cstring>

using namespace Inno;

namespace Inno
{
	void ExampleRenderingClientImpl::RegisterDevToggles()
	{
		// Getter reports the desired state — i.e. what the user's last click
		// asked for — so callers that read back immediately after Set() (the
		// IPC setter-reply convention) see their own write, not yesterday's
		// frame state. The frame loop reconciles Active with Desired at the
		// next boundary so the toggle never lands mid-frame.
		DevToggleRegistry::RegisterToggle("GPUPathTracer",
			[this]() { return m_GPUPathTracerDesired; },
			[this](bool desired) { m_GPUPathTracerDesired = desired; });

		// "GI on" reads/writes m_Bypassed across the rasterized-GI pass group.
		// Bypass landing per-frame (TASK-171); no Desired/Active reconciliation
		// needed because m_Bypassed is the source of truth read at dispatch.
		DevToggleRegistry::RegisterToggle("RasterizedGI",
			[]() {
				return !RadianceCacheRaytracingPass::Get().m_Bypassed.load(std::memory_order_relaxed);
			},
			[](bool desired) {
				const bool l_bypass = !desired;
				RadianceCacheReprojectionPass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
				RadianceCacheRaytracingPass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
				RadianceCacheFilterHorizontalPass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
				RadianceCacheFilterVerticalPass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
				RadianceCacheIntegrationPass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
				GIDenoisePass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
				GIFilterHorizontalPass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
				GIFilterVerticalPass::Get().m_Bypassed.store(l_bypass, std::memory_order_relaxed);
			});

		DevToggleRegistry::RegisterAction("Screenshot", [this]() { m_saveScreenCapture = true; });

		// TASK-183 runtime visualization-mode picker. One bool toggle per
		// mode; setting a mode true clears all the others (mutual exclusion
		// is the picker semantic — N booleans simulate a one-of-N enum).
		// Setter writes the picked uint to PerFrameDataService's atomic;
		// PerFrameDataService snapshots it into PerFrameConstantBuffer each
		// frame, lightPass.comp branches on g_Frame.debugViewMode.
		//
		// TASK-192 will replace these N booleans with a single Selector
		// primitive + dropdown in RenderTogglesPanel.vue. The engine-side
		// state (atomic uint in PerFrameDataService) does not change between
		// the two designs — only the registry-primitive shape and editor UX.
		struct DebugViewToggleEntry
		{
			const char*    m_ToggleName;
			DebugViewMode  m_Mode;
		};
		// TASK-205 trimmed the GBuffer modes — RenderTargetDebuggerPanel
		// covers raw-RT inspection by enumerating OpaquePass's m_ColorOutputs.
		// Survivors are the modes that need shader-side math.
		static const DebugViewToggleEntry s_DebugViewToggles[] = {
			{ "DebugView_DirectLightingOnly",     DebugViewMode::DirectLightingOnly },
			{ "DebugView_IndirectLightingOnly",   DebugViewMode::IndirectLightingOnly },
			{ "DebugView_SunShadowVisibility",    DebugViewMode::SunShadowVisibility },
			{ "DebugView_TileLightCountHeatmap",  DebugViewMode::TileLightCountHeatmap },
		};
		for (const auto& entry : s_DebugViewToggles)
		{
			const DebugViewMode l_Mode = entry.m_Mode;
			DevToggleRegistry::RegisterToggle(entry.m_ToggleName,
				[l_Mode]() {
					return g_Engine->Get<PerFrameDataService>()->GetDebugViewMode() == l_Mode;
				},
				[l_Mode](bool desired) {
					// Mutual exclusion: turning ON sets the mode; turning OFF
					// clears the mode only when the active mode is this one
					// (toggling another mode then this one off must not clobber
					// the other mode).
					auto* l_pfds = g_Engine->Get<PerFrameDataService>();
					if (desired)
					{
						l_pfds->SetDebugViewMode(l_Mode);
					}
					else if (l_pfds->GetDebugViewMode() == l_Mode)
					{
						l_pfds->SetDebugViewMode(DebugViewMode::None);
					}
				});
		}

		// TASK-195 runtime point-shadow bypass toggle. Binary feature-bypass
		// orthogonal to the DebugView picker above — flip on to force
		// EvaluateTiledPointLighting's inline-RT visibility=1 (unshadowed
		// reference) for the same-camera A/B per
		// .claude/disciplines/visual-validation.md. Replaces the former
		// compile-time #define DEBUG_POINT_SHADOW_BYPASS in
		// Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl.
		DevToggleRegistry::RegisterToggle("PointShadowBypass",
			[]() { return g_Engine->Get<PerFrameDataService>()->GetPointShadowBypass(); },
			[](bool desired) { g_Engine->Get<PerFrameDataService>()->SetPointShadowBypass(desired); });

		// TASK-195 env-var hookup mirroring INNO_RASTERIZED_GI's shape
		// (TASK-182). Lets a headless smoke run flip the bypass without
		// the editor in the loop:
		//   INNO_POINT_SHADOW_BYPASS=1 Main.exe -total_frames 80 ...
		if (const char* l_PointShadowBypassEnv = std::getenv("INNO_POINT_SHADOW_BYPASS"))
		{
			const bool l_BypassRequested = !(strcmp(l_PointShadowBypassEnv, "0") == 0
				|| strcmp(l_PointShadowBypassEnv, "off") == 0
				|| strcmp(l_PointShadowBypassEnv, "false") == 0);
			DevToggleRegistry::Set("PointShadowBypass", l_BypassRequested);
			Log(Success, "TASK-195 INNO_POINT_SHADOW_BYPASS='", l_PointShadowBypassEnv,
				"' applied; PointShadowBypass = ", l_BypassRequested ? "ON (visibility=1, A/B reference)" : "OFF (trace as normal).");
		}

		// TASK-183 CLI / env injection. Lets a headless smoke run pre-select
		// a debug-view mode without the editor in the loop:
		//   INNO_DEBUG_VIEW_MODE=DebugView_TileLightCountHeatmap Main.exe -total_frames 80 ...
		// Validates the runtime branch end-to-end (registry → PFDS atomic →
		// PerFrame_CB → lightPass.comp), which the editor IPC path also
		// exercises but is harder to drive from a smoke test.
		if (const char* l_DebugViewEnv = std::getenv("INNO_DEBUG_VIEW_MODE"))
		{
			if (DevToggleRegistry::Set(l_DebugViewEnv, true))
			{
				Log(Success, "TASK-183 INNO_DEBUG_VIEW_MODE='", l_DebugViewEnv,
					"' applied; PFDS mode = ",
					static_cast<uint32_t>(g_Engine->Get<PerFrameDataService>()->GetDebugViewMode()), ".");
			}
			else
			{
				Log(Warning, "INNO_DEBUG_VIEW_MODE='", l_DebugViewEnv,
					"' is not a registered DebugView toggle; ignoring.");
			}
		}

		// TASK-182 env-var hookup so the clear-on-bypass path can be smoke-
		// tested without the editor in the loop. INNO_RASTERIZED_GI=0 / "off"
		// / "false" disables the rasterized-GI group at startup, exercising
		// the bypass dispatch + RecordClearCommandList path the user
		// observed as "frozen GI on screen" before the fix.
		if (const char* l_RasterizedGIEnv = std::getenv("INNO_RASTERIZED_GI"))
		{
			const bool l_OnRequested = !(strcmp(l_RasterizedGIEnv, "0") == 0
				|| strcmp(l_RasterizedGIEnv, "off") == 0
				|| strcmp(l_RasterizedGIEnv, "false") == 0);
			DevToggleRegistry::Set("RasterizedGI", l_OnRequested);
			Log(Success, "TASK-182 INNO_RASTERIZED_GI='", l_RasterizedGIEnv,
				"' applied; RasterizedGI = ", l_OnRequested ? "ON" : "OFF (clear-on-bypass active).");
		}

		// TASK-77.4 CL-4 NRD anti-firefly live A/B. The setter writes through
		// to the dispatch-site read venue (NRDConstants.h::g_DenoiserSettings)
		// so the next frame's SetDenoiserSettings picks up the new value. The
		// other ReBLUR knobs (HitDistParams, MaxAccumulatedFrameNum,
		// LobeAngleFraction, RoughnessFraction) are not bool and thus not
		// representable through DevToggleRegistry's bool-only API; they are
		// edit-and-recompile tunables (see g_DenoiserSettings comment).
		// Registered unconditionally so the toggle list is stable across
		// NRD ON/OFF builds; the setter is a no-op write to a header-only
		// inline storage and stays valid even when ENABLED is false (the
		// engine just won't read the field on the OFF path).
		DevToggleRegistry::RegisterToggle("NRDAntiFirefly",
			[]() { return Inno::NRD::g_DenoiserSettings.EnableAntiFirefly; },
			[](bool desired) { Inno::NRD::g_DenoiserSettings.EnableAntiFirefly = desired; });
	}

	bool ExampleRenderingClientImpl::Setup(IServiceConfig* systemConfig)
	{
		RegisterDevToggles();

		if (strcmp(g_Engine->getInitConfig().testCase, "gpu_path_tracer") == 0)
		{
			m_GPUPathTracerDesired = true;
			m_GPUPathTracerActive  = true;
		}

		BootstrapAmbientCGTextures();

		BRDFLUTPass::Get().Setup();
		BRDFLUTMSPass::Get().Setup();

		SunShadowRTPass::Get().Setup();

		OpaqueCullingPass::Get().Setup();
		OpaquePass::Get().Setup();

		RadianceCacheReprojectionPass::Get().Setup();
		RadianceCacheRaytracingPass::Get().Setup();
		RadianceCacheFilterHorizontalPass::Get().Setup();
		RadianceCacheFilterVerticalPass::Get().Setup();
		RadianceCacheIntegrationPass::Get().Setup();
		GIDenoisePass::Get().Setup();
		GIFilterHorizontalPass::Get().Setup();
		GIFilterVerticalPass::Get().Setup();

		SSAOPass::Get().Setup();

		TiledFrustumGenerationPass::Get().Setup();
		LightCullingPass::Get().Setup();

		LightPass::Get().Setup();

		SkyPass::Get().Setup();

		PreTAAPass::Get().Setup();
		TAAPass::Get().Setup();

		LuminanceHistogramPass::Get().Setup();
		LuminanceAveragePass::Get().Setup();

		FinalBlendPass::Get().Setup();
		GPUPathTracerPass::Get().Setup();
		// PurgeTiles runs first each frame to free 50-frame-stale slots, so
		// the path tracer's InsertCell can claim them and UpdateTiles' early-
		// out skips them; UpdateTiles then resolves the path tracer's per-cell
		// scratch sums into the persistent ValueBuffer with a 16-sample-cap
		// running mean. `if constexpr` inside each pass elides everything when
		// the cache toggle is off, but the call still runs so the singleton
		// state flips to a benign Terminated.
		if constexpr (Inno::PTHashGridCache::ENABLED)
		{
			PTHashGridCachePurgeTilesPass::Get().Setup();
			PTHashGridCacheUpdateTilesPass::Get().Setup();
			PTHashGridCacheMipCascadeBuildPass::Get().Setup();
		}
		if constexpr (Inno::NRD::ENABLED)
		{
			PTNRDFormatConvertPass::Get().Setup();
			PTNRDDenoisePass::Get().Setup();
			PTNRDCompositionPass::Get().Setup();
		}

		// AnimationPass::Get().Setup();

		// TransparentGeometryProcessPass::Get().Setup();
		// TransparentBlendPass::Get().Setup();
		// VolumetricPass::Setup();

		// MotionBlurPass::Get().Setup();
		// BillboardPass::Get().Setup();
		// DebugPass::Get().Setup();

		// BSDFTestPass::Get().Setup();

		auto f_getUserPipelineOutputFunc = [this]()
			{
				return m_Canvas;
			};

	auto l_fmService = g_Engine->Get<FrameManagementService>();

		l_fmService->SetUserPipelineOutput(std::move(f_getUserPipelineOutputFunc));

		m_ObjectStatus = ObjectStatus::Created;

		return true;
	}
}

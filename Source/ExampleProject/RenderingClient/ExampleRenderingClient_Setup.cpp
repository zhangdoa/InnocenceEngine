#include "ExampleRenderingClient_Internal.h"
#include "SSAOPass.h"
#include "TiledFrustumGenerationPass.h"

#include "../../Engine/RenderGraph/RenderGraphService.h"

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
	}

	bool ExampleRenderingClientImpl::Setup(IServiceConfig* systemConfig)
	{
		RegisterDevToggles();

		BootstrapAmbientCGTextures();

		// The graph (loaded here, formerly in BRDFLUTPass::Setup) creates + owns
		// every node + resource. Only these two passes still run C++ Setup — to
		// create the imported resources that carry CPU init data.
		g_Engine->Get<RenderGraphService>()->LoadGraph("ExampleProject/RenderGraph/ExampleRenderGraph.json");

		SSAOPass::Get().Setup();
		TiledFrustumGenerationPass::Get().Setup();

		auto f_getUserPipelineOutputFunc = [this]()
			{
				return m_Canvas;
			};

	auto l_fmService = g_Engine->Get<FrameManagementService>();

		l_fmService->SetUserPipelineOutput(std::move(f_getUserPipelineOutputFunc));

		RegisterAuditCallback();

		m_ObjectStatus = ObjectStatus::Created;

		return true;
	}
}

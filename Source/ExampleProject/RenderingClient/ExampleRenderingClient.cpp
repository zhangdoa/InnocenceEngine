#pragma once
#include "ExampleRenderingClient.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SunShadowRTPass.h"
#include "OpaqueCullingPass.h"
#include "OpaquePass.h"
#include "AnimationPass.h"
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
#include "TransparentGeometryProcessPass.h"
#include "TransparentBlendPass.h"
#include "VolumetricPass.h"
#include "TAAPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"
#include "GPUPathTracerPass.h"
#include "PTHashGridCachePurgeTilesPass.h"
#include "PTHashGridCacheUpdateTilesPass.h"
#include "PTHashGridCacheMipCascadeBuildPass.h"
#include "HashGridCacheConstants.h"

#include "BSDFTestPass.h"

#include "../../Engine/Services/HIDService.h"
#include "../../Engine/Services/DevToggleRegistry.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/ViewportSourceOverride.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Services/EditorService.h"
#include "../../Engine/Common/IOService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Common/Task.h"
#include "../../Engine/Common/TaskScheduler.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

using namespace Inno;

namespace Inno
{
	// Bypass helpers live in a sibling .inl to keep ExampleRenderingClient.cpp
	// under the file-size soft ratchet (.claude/disciplines/split-before-grow.md).
	#include "ExampleRenderingClient_Bypass.inl"

	class ExampleRenderingClientImpl : public IRenderingClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ExampleRenderingClientImpl);

		// Inherited via IRenderingClient
		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool PrepareCommands() override;
		bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
		bool FinalizeGPUResults() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		// Readback the final blend output and save gpu_output.png. Callable
		// from any point where the GPU is still alive (per-frame trigger or
		// the Engine::Terminate GPU-finalization phase). Idempotent — sets
		// m_autoCaptureWritten so later calls no-op.
		void TryWriteAutoCapture();

		// GPU-to-PNG writer used by both the one-shot auto-capture and the
		// per-frame `-dump_frames START-END` path. Synchronises on the
		// graphics queue, reads FinalBlendPass output back to CPU, converts
		// to sRGB 8-bit, and writes the file. Returns false on empty
		// readback or save failure; not idempotent — caller owns any
		// "already wrote" latching.
		bool WriteCaptureToFile(const char* filename);

		bool m_drawBRDFTest = false;
		// Desired is what the user/toggle asked for; Active is what the
		// frame loop has actually switched into. PrepareCommands reconciles
		// them at the next frame boundary so the toggle never lands mid-frame.
		bool m_GPUPathTracerDesired = false;
		bool m_GPUPathTracerActive  = false;
		bool m_saveScreenCapture = false;


		uint32_t m_autoCaptureFrameCount = 0;
		bool m_autoCaptureWritten = false;

		GPUResourceComponent* m_Canvas;
		RenderPassComponent* m_CanvasOwner;

		bool m_ExecuteOneShotCommands = true;

		void AuditDump();

	private:
		ObjectStatus m_ObjectStatus;
	};

	bool ExampleRenderingClientImpl::Setup(IServiceConfig* systemConfig)
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

		if (strcmp(g_Engine->getInitConfig().testCase, "gpu_path_tracer") == 0)
		{
			m_GPUPathTracerDesired = true;
			m_GPUPathTracerActive  = true;
		}

		// Idempotent bootstrap of AmbientCG PBR sets that materials in
		// ExampleProject scenes reference. Each ImportTexture writes a
		// TextureComponent JSON + binary if the JSON is missing; existing
		// JSONs short-circuit the load → recompress → save chain.
		struct PBRSetSlot { const char* slotName; uint32_t slotIndex; bool isSRGB; };
		static const PBRSetSlot s_AmbientCGSlots[] = {
			{ "NormalGL",  0u, false },
			{ "Color",     1u, true  },
			{ "Metalness", 2u, false },
			{ "Roughness", 3u, false },
		};
		static const char* s_AmbientCGSets[] = { "Concrete007", "Ground037", "Metal032", "Tiles074" };
		auto* l_io = g_Engine->Get<IOService>();
		auto l_dataDir = l_io->getDataDirectory();
		for (const char* setName : s_AmbientCGSets)
		{
			std::string l_setDir = std::string("../OriginalAssets/Textures/") + setName + "/";
			std::string l_baseName = std::string(setName) + "_1K-PNG";
			for (const auto& slot : s_AmbientCGSlots)
			{
				std::string l_pngPath = l_setDir + l_baseName + "_" + slot.slotName + ".png";
				if (!l_io->isFileExist(l_pngPath.c_str()))
					continue;

				std::string l_instanceName = std::string(setName) + "_" + slot.slotName + ".TextureComponent";

				auto l_destJson = l_dataDir + AssetService::GetAssetFilePath(l_instanceName.c_str());
				if (l_io->isFileExist(l_destJson.c_str()))
					continue;

				std::string l_absPath = l_dataDir + l_pngPath;
				AssetService::ImportTexture(l_absPath.c_str(),
					TextureSampler::Sampler2D, TextureUsage::Sample,
					slot.isSRGB, slot.slotIndex, l_instanceName.c_str());
			}
		}

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

	bool ExampleRenderingClientImpl::Initialize()
	{

		BRDFLUTPass::Get().Initialize();
		BRDFLUTMSPass::Get().Initialize();

		SunShadowRTPass::Get().Initialize();

		OpaqueCullingPass::Get().Initialize();
		OpaquePass::Get().Initialize();

		RadianceCacheReprojectionPass::Get().Initialize();
		RadianceCacheRaytracingPass::Get().Initialize();
		RadianceCacheFilterHorizontalPass::Get().Initialize();
		RadianceCacheFilterVerticalPass::Get().Initialize();
		RadianceCacheIntegrationPass::Get().Initialize();
		GIDenoisePass::Get().Initialize();
		GIFilterHorizontalPass::Get().Initialize();
		GIFilterVerticalPass::Get().Initialize();

		SSAOPass::Get().Initialize();

		TiledFrustumGenerationPass::Get().Initialize();
		LightCullingPass::Get().Initialize();

		LightPass::Get().Initialize();

		SkyPass::Get().Initialize();

		PreTAAPass::Get().Initialize();
		TAAPass::Get().Initialize();

		LuminanceHistogramPass::Get().Initialize();
		LuminanceAveragePass::Get().Initialize();

		FinalBlendPass::Get().Initialize();
		GPUPathTracerPass::Get().Initialize();
		if constexpr (Inno::PTHashGridCache::ENABLED)
		{
			PTHashGridCachePurgeTilesPass::Get().Initialize();
			PTHashGridCacheUpdateTilesPass::Get().Initialize();
			PTHashGridCacheMipCascadeBuildPass::Get().Initialize();
		}

		m_ObjectStatus = ObjectStatus::Activated;

		return true;
	}

	bool ExampleRenderingClientImpl::Update()
	{
		RadianceCacheReprojectionPass::Get().Update();
		TiledFrustumGenerationPass::Get().Update();
		LightCullingPass::Get().Update();
		LuminanceAveragePass::Get().Update();

		if (m_GPUPathTracerActive)
		{
			GPUPathTracerPass::Get().Update();
			if constexpr (Inno::PTHashGridCache::ENABLED)
			{
				PTHashGridCachePurgeTilesPass::Get().Update();
				PTHashGridCacheUpdateTilesPass::Get().Update();
				PTHashGridCacheMipCascadeBuildPass::Get().Update();
			}
		}

		return true;
	}

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
			// scratch deltas into ValueBuffer at mip 0 so the path tracer
			// reads the freshest running mean; MipCascadeBuild then aggregates
			// 2x2 children at each level into mips 1-3 so wide-footprint
			// reads (next CL) can pick a level matching their footprint. Each
			// pass is a no-op when the cache toggle is off.
			if constexpr (Inno::PTHashGridCache::ENABLED)
			{
				DispatchOrBypass(PTHashGridCachePurgeTilesPass::Get());
				DispatchOrBypass(PTHashGridCacheUpdateTilesPass::Get());
				DispatchOrBypass(PTHashGridCacheMipCascadeBuildPass::Get());
			}
			DispatchOrBypass(GPUPathTracerPass::Get());
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

	bool ExampleRenderingClientImpl::ExecuteCommands(IRenderingConfig* renderingConfig)
	{
		auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		auto l_fmService = g_Engine->Get<FrameManagementService>();
		GPUResourceComponent* l_canvas;
		RenderPassComponent* l_canvasOwner;
		if (m_ExecuteOneShotCommands)
		{
			if (BRDFLUTPass::Get().GetStatus() == ObjectStatus::Activated
				&& BRDFLUTMSPass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(BRDFLUTPass::Get())
				&& !IsBypassed(BRDFLUTMSPass::Get()))
			{
				auto l_brdfRenderPass = BRDFLUTPass::Get().GetRenderPassComp();

				auto l_brdfPassComputeCommandList = BRDFLUTPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_brdfPassComputeCommandList, GPUEngineType::Compute);

				l_hwService->SignalOnGPU(l_brdfRenderPass, GPUEngineType::Compute);
				l_hwService->WaitOnGPU(l_brdfRenderPass, GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_brdfMSCommandList = BRDFLUTMSPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_brdfMSCommandList, GPUEngineType::Compute);
				auto l_brdfMSRenderPass = BRDFLUTMSPass::Get().GetRenderPassComp();
				l_hwService->SignalOnGPU(l_brdfMSRenderPass, GPUEngineType::Compute);

				auto l_graphicsSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
				auto l_computeSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Compute);
				l_hwService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
				l_hwService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
				m_ExecuteOneShotCommands = false;
			}
		}

		// PurgeTiles → UpdateTiles → MipCascadeBuild → PathTracer chain.
		// PurgeTiles must complete before UpdateTiles (the latter's
		// HashBuffer == 0 early-out must see freed slots, otherwise stale-
		// tile scratch contributes to running-mean) and before the path
		// tracer (so InsertCell can claim freed slots). UpdateTiles must
		// complete before MipCascadeBuild (the cascade reads the mip-0
		// values UpdateTiles just resolved). MipCascadeBuild is a UAV
		// writer of ValueBuffer at mips 1-3; the path tracer does not
		// read those mip-N cells this CL (Site-3 read still uses mip 0)
		// so the Wait on UpdateTiles before the path tracer is sufficient
		// for correctness — but we still serialise MipCascadeBuild against
		// the path tracer to keep the chain order intact for the next CL,
		// which adds a mip-aware read at the same site. All four run on
		// the Compute queue, so same-queue Signal/Wait pairs are
		// sufficient — no graphics-side fence. Toggle-off keeps the entire
		// block elided at compile time (each pass stays Terminated, the
		// inner gates short-circuit regardless), preserving the bit-
		// identical bypass.
		if constexpr (Inno::PTHashGridCache::ENABLED)
		{
			if (m_GPUPathTracerActive
				&& PTHashGridCachePurgeTilesPass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(PTHashGridCachePurgeTilesPass::Get()))
			{
				auto l_renderPass = PTHashGridCachePurgeTilesPass::Get().GetRenderPassComp();
				auto l_computeCL  = PTHashGridCachePurgeTilesPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
			}

			if (m_GPUPathTracerActive
				&& PTHashGridCacheUpdateTilesPass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(PTHashGridCacheUpdateTilesPass::Get()))
			{
				// Wait on PurgeTiles before merging running mean — the
				// HashBuffer == 0 early-out must observe freed slots.
				WaitIfActive(PTHashGridCachePurgeTilesPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_renderPass = PTHashGridCacheUpdateTilesPass::Get().GetRenderPassComp();
				auto l_computeCL  = PTHashGridCacheUpdateTilesPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
			}

			if (m_GPUPathTracerActive
				&& PTHashGridCacheMipCascadeBuildPass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(PTHashGridCacheMipCascadeBuildPass::Get()))
			{
				// Wait on UpdateTiles before aggregating — the mip-0
				// values must be the freshly-resolved running mean, not
				// the previous frame's stale ValueBuffer entries.
				WaitIfActive(PTHashGridCacheUpdateTilesPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_renderPass = PTHashGridCacheMipCascadeBuildPass::Get().GetRenderPassComp();
				auto l_computeCL  = PTHashGridCacheMipCascadeBuildPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
			}
		}

		if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(GPUPathTracerPass::Get()))
		{
			auto l_renderPass = GPUPathTracerPass::Get().GetRenderPassComp();

			// Graphics CL: transition accumulation buffer to UAV
			auto l_graphicsCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCL, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Compute CL: ray tracing dispatch (also transitions AccumBuffer to ReadOnly at end).
			// Wait on MipCascadeBuild — last link in the cache chain — so this
			// frame's reads see both the resolved running mean (mip 0) and
			// the freshly-aggregated coarser cells (mips 1-3, written but
			// unread this CL; the next CL adds mip-aware Site-3 lookup).
			// The same-queue Signal/Wait chain transitively covers the
			// upstream PurgeTiles + UpdateTiles waits.
			if constexpr (Inno::PTHashGridCache::ENABLED)
				WaitIfActive(PTHashGridCacheMipCascadeBuildPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);
			auto l_computeCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (!m_GPUPathTracerActive)
		{

		if (OpaqueCullingPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(OpaqueCullingPass::Get()))
		{
			auto l_commandList = OpaqueCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = OpaqueCullingPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (OpaquePass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(OpaquePass::Get()))
		{
			WaitIfActive(OpaqueCullingPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			auto l_commandList = OpaquePass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_commandList, GPUEngineType::Graphics);
			auto l_renderPass = OpaquePass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
		}

		// TASK-138 phase 1: RT sun-shadow dispatch. Same wait/signal pattern
		// as RadianceCacheRaytracingPass — graphics CL transitions resources,
		// signals the renderpass; compute CL waits on that fence + on
		// OpaquePass (needs GBuffer position/normal), executes ray dispatch,
		// signals own renderpass for LightPass to wait on.
		if (SunShadowRTPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SunShadowRTPass::Get()))
		{
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = SunShadowRTPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SunShadowRTPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SunShadowRTPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheReprojectionPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(RadianceCacheReprojectionPass::Get()))
		{
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = RadianceCacheReprojectionPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheReprojectionPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheReprojectionPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheRaytracingPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(RadianceCacheRaytracingPass::Get()))
		{
			WaitIfActive(RadianceCacheReprojectionPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheRaytracingPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheRaytracingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			auto l_computeCommandList = RadianceCacheRaytracingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheFilterHorizontalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(RadianceCacheFilterHorizontalPass::Get()))
		{
			WaitIfActive(RadianceCacheRaytracingPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheFilterHorizontalPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheFilterVerticalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(RadianceCacheFilterVerticalPass::Get()))
		{
			WaitIfActive(RadianceCacheFilterHorizontalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheFilterVerticalPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheIntegrationPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(RadianceCacheIntegrationPass::Get()))
		{
			WaitIfActive(RadianceCacheFilterVerticalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheIntegrationPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheIntegrationPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheIntegrationPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (GIDenoisePass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(GIDenoisePass::Get()))
		{
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			WaitIfActive(RadianceCacheIntegrationPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = GIDenoisePass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = GIDenoisePass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = GIDenoisePass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (GIFilterHorizontalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(GIFilterHorizontalPass::Get()))
		{
			WaitIfActive(GIDenoisePass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = GIFilterHorizontalPass::Get().GetRenderPassComp();
			l_hwService->Execute(GIFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Graphics), GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			l_hwService->Execute(GIFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Compute), GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (GIFilterVerticalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(GIFilterVerticalPass::Get()))
		{
			WaitIfActive(GIFilterHorizontalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = GIFilterVerticalPass::Get().GetRenderPassComp();
			l_hwService->Execute(GIFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Graphics), GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			l_hwService->Execute(GIFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Compute), GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSAOPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSAOPass::Get()))
		{
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = SSAOPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSAOPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSAOPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (TiledFrustumGenerationPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(TiledFrustumGenerationPass::Get()))
		{
			auto l_commandList = TiledFrustumGenerationPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = TiledFrustumGenerationPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LightCullingPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(LightCullingPass::Get()))
		{
			WaitIfActive(TiledFrustumGenerationPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = LightCullingPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LightPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(LightPass::Get()))
		{
			// TASK-138: wait on RT sun-shadow dispatch so the visibility texture
			// is consumable when LightPass binds slot t13. Suspended (e.g. early
			// frames before TLAS build) means LightPass binds nullptr and the
			// sun is treated as fully shadowed for that frame.
			WaitIfActive(SunShadowRTPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			WaitIfActive(SSAOPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			WaitIfActive(LightCullingPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			if (GIFilterVerticalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(GIFilterVerticalPass::Get()))
				WaitIfActive(GIFilterVerticalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			else
				WaitIfActive(GIDenoisePass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = LightPass::Get().GetRenderPassComp();
			
			auto l_graphicsCommandList = LightPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			
			auto l_computeCommandList = LightPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SkyPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SkyPass::Get()))
		{
			auto l_commandList = SkyPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = SkyPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (PreTAAPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(PreTAAPass::Get()))
		{
			WaitIfActive(LightPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			WaitIfActive(SkyPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = PreTAAPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = PreTAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = PreTAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (TAAPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(TAAPass::Get()))
		{
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			WaitIfActive(PreTAAPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = TAAPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = TAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = TAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		} // end if (!m_GPUPathTracerActive)

		if (LuminanceHistogramPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(LuminanceHistogramPass::Get()))
		{
			if (m_GPUPathTracerActive)
				WaitIfActive(GPUPathTracerPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			else
				WaitIfActive(TAAPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = LuminanceHistogramPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LuminanceAveragePass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(LuminanceAveragePass::Get()))
		{
			WaitIfActive(LuminanceHistogramPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);
			auto l_commandList = LuminanceAveragePass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = LuminanceAveragePass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (FinalBlendPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(FinalBlendPass::Get()))
		{
			if (m_GPUPathTracerActive)
				WaitIfActive(GPUPathTracerPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			else
				WaitIfActive(TAAPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			WaitIfActive(LuminanceAveragePass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = FinalBlendPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (m_saveScreenCapture)
		{
			// TASK-211: editor Screenshot action consumer. Writes a uniquely-
			// named capture under Bin/Captures/Screenshots/ (engine CWD-relative)
			// and broadcasts SCREENSHOT_SAVED via EditorService so the editor
			// can surface a toast naming the absolute path. Best-effort
			// broadcast — the engine-side log line is the durable record.
			auto l_srcTextureComp = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
			auto* l_editorService = g_Engine->Get<EditorService>();

			// Step 1: ensure output directory exists.
			const std::filesystem::path l_outputDir = std::filesystem::path("Captures") / "Screenshots";
			std::error_code l_dirEc;
			std::filesystem::create_directories(l_outputDir, l_dirEc);
			if (l_dirEc)
			{
				const std::string l_errorReason =
					std::string("Screenshot: failed to create directory '") + l_outputDir.string()
					+ "': " + l_dirEc.message();
				Log(Warning, "Screenshot failed: ", l_errorReason.c_str());
				(void)l_editorService->BroadcastScreenshotSaved(false, std::string(), l_errorReason);
				m_saveScreenCapture = false;
			}
			else
			{
				// Step 2: timestamped filename. Millisecond precision avoids
				// collisions when the user clicks twice within the same second.
				const auto l_now = std::chrono::system_clock::now();
				const auto l_nowTimeT = std::chrono::system_clock::to_time_t(l_now);
				const auto l_nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					l_now.time_since_epoch()) % std::chrono::milliseconds(1000);
				std::tm l_tm{};
#if defined(_WIN32)
				localtime_s(&l_tm, &l_nowTimeT);
#else
				localtime_r(&l_nowTimeT, &l_tm);
#endif
				std::ostringstream l_nameStream;
				l_nameStream << "screenshot_"
					<< std::put_time(&l_tm, "%Y-%m-%d_%H-%M-%S")
					<< "-" << std::setw(3) << std::setfill('0') << l_nowMs.count();

				// Step 3: extension by pixel-format branch — matches
				// STBWrapper::Save (UByte -> stbi_write_png; Float16/Float32
				// -> stbi_write_hdr).
				const TexturePixelDataType l_pixelType = l_srcTextureComp->m_TextureDesc.PixelDataType;
				const char* l_extension = (l_pixelType == TexturePixelDataType::Float16
					|| l_pixelType == TexturePixelDataType::Float32) ? ".hdr" : ".png";
				l_nameStream << l_extension;

				const std::filesystem::path l_relativePath = l_outputDir / l_nameStream.str();
				std::error_code l_absEc;
				const std::filesystem::path l_absolutePath =
					std::filesystem::absolute(l_relativePath, l_absEc).make_preferred();
				const std::string l_absolutePathStr = l_absEc
					? std::filesystem::path(l_relativePath).make_preferred().string()
					: l_absolutePath.string();

				// Step 4: GPU readback.
				auto l_textureData = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
					FinalBlendPass::Get().GetRenderPassComp(), l_srcTextureComp);
				if (l_textureData.empty())
				{
					const std::string l_errorReason =
						std::string("Screenshot: ReadTextureBackToCPU returned empty for '")
						+ l_absolutePathStr + "'.";
					Log(Warning, "Screenshot failed: ", l_errorReason.c_str());
					(void)l_editorService->BroadcastScreenshotSaved(false, l_absolutePathStr, l_errorReason);
				}
				else if (g_Engine->Get<AssetService>()->Save(l_absolutePathStr.c_str(),
					l_srcTextureComp->m_TextureDesc, l_textureData.data()))
				{
					Log(Success, "Screenshot: ", l_absolutePathStr.c_str());
					(void)l_editorService->BroadcastScreenshotSaved(true, l_absolutePathStr, std::string());
				}
				else
				{
					const std::string l_errorReason =
						std::string("Screenshot: AssetService::Save failed for '")
						+ l_absolutePathStr + "'.";
					Log(Warning, "Screenshot failed: ", l_errorReason.c_str());
					(void)l_editorService->BroadcastScreenshotSaved(false, l_absolutePathStr, l_errorReason);
				}
				m_saveScreenCapture = false;
			}
		}

		auto l_totalFrames = g_Engine->getInitConfig().totalFrames;
		const bool l_isPathTracerTestMode =
			strcmp(g_Engine->getInitConfig().testCase, "gpu_path_tracer") == 0 && m_GPUPathTracerActive;
		// Serialize-test mode sets totalFrames=1 for the parse's auto-terminate
		// path, but the render pipeline (FinalBlendPass, et al.) is intentionally
		// not activated in that mode — the test's whole work is scene load +
		// save + compare, no rendering. Skip the auto-capture trigger so
		// ReadTextureBackToCPU doesn't run against a texture with empty GPU
		// resources and hit the fatal-on-error log path.
		const bool l_isSerializeTest = g_Engine->getInitConfig().serializeTest[0] != '\0';
		const uint32_t l_triggerAtFrame = l_isSerializeTest ? 0u
			: (l_totalFrames > 0
				? static_cast<uint32_t>(l_totalFrames)
				: (l_isPathTracerTestMode ? 30u : 0u));

		// Unified per-frame counter. Previously lived inside the one-shot
		// trigger's conditional; moved out so the `-dump_frames` path can
		// share it. Runs that don't use either feature increment the
		// counter harmlessly — nothing else reads it.
		// TASK-213 CL B: gated on FrameManagementService's steady-state latch
		// so the counter is steady-state-relative, not absolute. Frozen at 0
		// until IsSteadyState() first goes true; from that frame on, advances
		// 1-per-rendered-frame. Cross-launch the load-frame count varies
		// (deferred-init drain timing) so the absolute counter at the dump
		// frame would differ; the latch gates that variability out. Flap-back
		// (TLAS rebuild after first-true, e.g. GISponza frame=16 / 30) does
		// NOT reset the counter — once accumulation has begun, resetting would
		// corrupt the running mean. See CL A's reviewer carry-forward.
		if (g_Engine->Get<FrameManagementService>()->HasReachedSteadyState())
			m_autoCaptureFrameCount++;

		// Frame-sequence dump for temporal validation. When
		// `-dump_frames START-END` is set, write `gpu_output_NNNN.png` for
		// every frame N in [START, END] inclusive. Lets a reviewer scrub /
		// diff consecutive frames to catch flickering, probe-spawn
		// oscillation, or denoiser instability that a single-frame capture
		// can't expose. Skipped when the serialize-test or an un-activated
		// FinalBlendPass would make the readback meaningless (same guard
		// shape as the one-shot trigger).
		const auto& l_initCfg = g_Engine->getInitConfig();
		if (!l_isSerializeTest
			&& l_initCfg.dumpFramesStart >= 0
			&& l_initCfg.dumpFramesEnd >= l_initCfg.dumpFramesStart
			&& m_autoCaptureFrameCount >= static_cast<uint32_t>(l_initCfg.dumpFramesStart)
			&& m_autoCaptureFrameCount <= static_cast<uint32_t>(l_initCfg.dumpFramesEnd))
		{
			char l_buf[64];
			snprintf(l_buf, sizeof(l_buf), "gpu_output_%04u.png", m_autoCaptureFrameCount);
			WriteCaptureToFile(l_buf);
		}

		// Per-frame trigger: mid-session snapshot (e.g. path tracer frame 30).
		// The structural fallback is FinalizeGPUResults, which runs at shutdown
		// after WaitForGPUIdle and catches any case the per-frame trigger missed
		// (e.g. the user exited before the trigger frame). TASK-42.
		if (l_triggerAtFrame > 0 && !m_autoCaptureWritten)
		{
			if (m_autoCaptureFrameCount >= l_triggerAtFrame)
				TryWriteAutoCapture();
		}

		if (g_Engine->getInitConfig().isAudit)
		{
			static uint32_t s_AuditFrame = 0;
			if (++s_AuditFrame == 5)
				AuditDump();
		}

		return true;
	}

	bool ExampleRenderingClientImpl::WriteCaptureToFile(const char* filename)
	{
		auto l_fmService = g_Engine->Get<FrameManagementService>();
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		l_hwService->SignalOnGPU(l_fmService->GetGlobalSemaphore(), GPUEngineType::Graphics);
		auto l_semVal = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_semVal, GPUEngineType::Graphics);

		auto l_srcTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto l_texFrameIndex = l_srcTex->m_TextureDesc.IsMultiBuffer ? l_fmService->GetCurrentFrame() : 0u;
		l_srcTex->SetCurrentState(l_texFrameIndex, l_srcTex->m_WriteState);
		auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
			FinalBlendPass::Get().GetRenderPassComp(), l_srcTex);

		if (l_floatPixels.empty())
		{
			Log(Warning, "Capture: ReadTextureBackToCPU returned empty for ", filename);
			return false;
		}

		std::vector<uint8_t> l_uint8Pixels;
		l_uint8Pixels.reserve(l_floatPixels.size() * 4);
		for (const auto& px : l_floatPixels)
		{
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.x, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.y, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.z, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255));
		}

		TextureDesc l_desc = l_srcTex->m_TextureDesc;
		l_desc.PixelDataType = TexturePixelDataType::UByte;
		l_desc.PixelDataFormat = TexturePixelDataFormat::RGBA;
		l_desc.Sampler = TextureSampler::Sampler2D;

		if (g_Engine->Get<AssetService>()->Save(filename, l_desc, l_uint8Pixels.data()))
		{
			Log(Success, "Capture: ", filename, " written.");
			return true;
		}
		Log(Warning, "Capture: failed to write ", filename);
		return false;
	}

	void ExampleRenderingClientImpl::TryWriteAutoCapture()
	{
		if (m_autoCaptureWritten)
			return;
		m_autoCaptureWritten = true;

		// PathTracerReadback stats: zero/non-zero pixel split, mean, max.
		// Originally added for path-tracer convergence diagnosis; kept here
		// (one-shot path) rather than in the per-frame dump path so a
		// 100-frame `-dump_frames` run doesn't spam the log.
		auto l_srcTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
			FinalBlendPass::Get().GetRenderPassComp(), l_srcTex);
		if (!l_floatPixels.empty())
		{
			size_t l_zeroCount = 0, l_nonZeroCount = 0;
			float l_sumR = 0, l_sumG = 0, l_sumB = 0, l_maxR = 0, l_maxG = 0, l_maxB = 0;
			for (const auto& px : l_floatPixels)
			{
				const bool l_isZero = (px.x == 0.0f && px.y == 0.0f && px.z == 0.0f);
				if (l_isZero) { l_zeroCount++; }
				else
				{
					l_nonZeroCount++;
					l_sumR += px.x; l_sumG += px.y; l_sumB += px.z;
					if (px.x > l_maxR) l_maxR = px.x;
					if (px.y > l_maxG) l_maxG = px.y;
					if (px.z > l_maxB) l_maxB = px.z;
				}
			}
			const float l_total = static_cast<float>(l_floatPixels.size());
			Log(Success, "PathTracerReadback: total=", l_floatPixels.size(),
				" zero=", l_zeroCount, " nonZero=", l_nonZeroCount,
				" mean=(", l_sumR / l_total, ",", l_sumG / l_total, ",", l_sumB / l_total, ")",
				" max=(", l_maxR, ",", l_maxG, ",", l_maxB, ")");
		}

		WriteCaptureToFile("gpu_output.png");
	}

	bool ExampleRenderingClientImpl::FinalizeGPUResults()
	{
		// Structural fallback for the per-frame auto-capture trigger in
		// ExecuteCommands. Engine::Terminate calls this AFTER WaitForGPUIdle
		// and BEFORE the LogicClient CPU path tracer, so the GPU is guaranteed
		// alive and a readback/PNG write here is safe. Per-frame trigger
		// usually wins (m_autoCaptureWritten is already set); this catches
		// the "user exited before the trigger frame fired" case.
		// Serialize-test mode sets totalFrames=1 as an auto-terminate signal
		// but doesn't render; skip the capture path so it doesn't try to
		// read back an unactivated texture at shutdown.
		const bool l_isSerializeTest = g_Engine->getInitConfig().serializeTest[0] != '\0';
		if (g_Engine->getInitConfig().totalFrames > 0 && !m_autoCaptureWritten && !l_isSerializeTest)
			TryWriteAutoCapture();
		return true;
	}

	void ExampleRenderingClientImpl::AuditDump()
	{
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		
		l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);

		auto Dump = [&](const char* filename, RenderPassComponent* rp, TextureComponent* tc)
		{
			if (!tc) { Log(Warning, "AuditDump: null TextureComponent for ", filename); return; }
			auto l_pixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(rp, tc);
			if (l_pixels.empty()) { Log(Error, "AuditDump: empty readback for ", filename); return; }
			for (size_t pi = 0; pi < l_pixels.size(); pi++) {
				if (l_pixels[pi].x != 0.0f || l_pixels[pi].y != 0.0f || l_pixels[pi].z != 0.0f) {
					uint32_t py = (uint32_t)(pi / tc->m_TextureDesc.Width);
					uint32_t px = (uint32_t)(pi % tc->m_TextureDesc.Width);
					Log(Verbose, "AuditDump: ", filename, " first-nonzero y=", py, " x=", px, " rgba=(", l_pixels[pi].x, ",", l_pixels[pi].y, ",", l_pixels[pi].z, ",", l_pixels[pi].w, ")");
					break;
				}
			}
			TextureDesc l_desc = tc->m_TextureDesc;
			l_desc.PixelDataType = TexturePixelDataType::Float32;
			g_Engine->Get<AssetService>()->Save(filename, l_desc, l_pixels.data());
			Log(Success, "AuditDump: saved ", filename);
		};

		auto DumpRP = [&](const char* filename, RenderPassComponent* rp, uint32_t colorIndex = 0)
		{
			if (!rp || !rp->m_OutputMergerTarget) { Log(Warning, "AuditDump: null RenderPassComp for ", filename); return; }
			if (colorIndex >= rp->m_OutputMergerTarget->m_ColorOutputs.size()) { Log(Warning, "AuditDump: RT index ", colorIndex, " out of range for ", filename); return; }
			Dump(filename, rp, rp->m_OutputMergerTarget->m_ColorOutputs[colorIndex]);
		};

		// 1-2: BRDF LUTs
		Dump("audit_01_BRDFLUTPass.hdr",   BRDFLUTPass::Get().GetRenderPassComp(),   static_cast<TextureComponent*>(BRDFLUTPass::Get().GetResult()));
		Dump("audit_02_BRDFLUTMSPass.hdr",  BRDFLUTMSPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(BRDFLUTMSPass::Get().GetResult()));

		// 3: Sun shadow R8 visibility texture from SunShadowRTPass.
		if (SunShadowRTPass::Get().GetStatus() == ObjectStatus::Activated)
			Dump("audit_03c_SunShadowRT.hdr",
				SunShadowRTPass::Get().GetRenderPassComp(),
				SunShadowRTPass::Get().GetResult());

		// 4: Opaque G-buffer
		DumpRP("audit_04a_Opaque_RT0.hdr", OpaquePass::Get().GetRenderPassComp(), 0);
		DumpRP("audit_04b_Opaque_RT1.hdr", OpaquePass::Get().GetRenderPassComp(), 1);
		DumpRP("audit_04c_Opaque_RT2.hdr", OpaquePass::Get().GetRenderPassComp(), 2);

		// 5: SSAO
		Dump("audit_05_SSAO.hdr", SSAOPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(SSAOPass::Get().GetResult()));

		// 8: Light
		Dump("audit_08a_Light_Luminance.hdr",   LightPass::Get().GetRenderPassComp(), LightPass::Get().GetLuminanceResult());
		Dump("audit_08b_Light_Illuminance.hdr",  LightPass::Get().GetRenderPassComp(), LightPass::Get().GetIlluminanceResult());

		// 9: Sky
		Dump("audit_09_Sky.hdr",     SkyPass::Get().GetRenderPassComp(),  static_cast<TextureComponent*>(SkyPass::Get().GetResult()));

		// 10: TAA
		Dump("audit_10_TAAPass.hdr", TAAPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(TAAPass::Get().GetResult()));

		// 11: Final blend
		// PrepareSwapChainCommands (which runs before ExecuteCommands) speculatively
		// transitions FinalBlend result to SRV in the state tracker. At AuditDump time
		// the actual GPU state is UAV (compute wrote, swap chain hasn't executed yet).
		// Temporarily correct the tracker, dump, then restore so the swap chain CL works.
		{
			auto* l_fbTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
			auto l_fbIdx = l_fbTex->m_TextureDesc.IsMultiBuffer ? g_Engine->Get<FrameManagementService>()->GetCurrentFrame() : 0u;
			auto l_savedState = l_fbTex->GetCurrentState(l_fbIdx);
			l_fbTex->SetCurrentState(l_fbIdx, l_fbTex->m_WriteState);
			Dump("audit_11_FinalBlend.hdr", FinalBlendPass::Get().GetRenderPassComp(), l_fbTex);
			l_fbTex->SetCurrentState(l_fbIdx, l_savedState);
		}

		Log(Success, "AuditDump complete. Check Bin/*.hdr");
		std::exit(0);
	}

	bool ExampleRenderingClientImpl::Terminate()
	{
		// Registered callbacks capture `this`; clear the registry before this
		// instance starts to die so an in-flight WS message can't deref it.
		DevToggleRegistry::Clear();

		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		auto l_graphicsSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);

		// Auto-capture readback now happens in Update() on the last frame,
		// before the CPU path tracer runs and causes a GPU device timeout.

		FinalBlendPass::Get().Terminate();

		LuminanceAveragePass::Get().Terminate();
		LuminanceHistogramPass::Get().Terminate();

		TAAPass::Get().Terminate();
		PreTAAPass::Get().Terminate();

		SkyPass::Get().Terminate();

		LightPass::Get().Terminate();
		GIFilterVerticalPass::Get().Terminate();
		GIFilterHorizontalPass::Get().Terminate();
		GIDenoisePass::Get().Terminate();

		LightCullingPass::Get().Terminate();
		TiledFrustumGenerationPass::Get().Terminate();

		SSAOPass::Get().Terminate();

		RadianceCacheIntegrationPass::Get().Terminate();
		RadianceCacheFilterHorizontalPass::Get().Terminate();
		RadianceCacheFilterVerticalPass::Get().Terminate();
		RadianceCacheRaytracingPass::Get().Terminate();
		RadianceCacheReprojectionPass::Get().Terminate();

		OpaqueCullingPass::Get().Terminate();
		OpaquePass::Get().Terminate();

		SunShadowRTPass::Get().Terminate();

		BRDFLUTMSPass::Get().Terminate();
		BRDFLUTPass::Get().Terminate();

		m_ObjectStatus = ObjectStatus::Terminated;

		return true;
	}

	ObjectStatus ExampleRenderingClientImpl::GetStatus()
	{
		return m_ObjectStatus;
	}
}

bool ExampleRenderingClient::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new ExampleRenderingClientImpl();
	return m_Impl->Setup(systemConfig);
}

bool ExampleRenderingClient::Initialize()
{
	return m_Impl->Initialize();
}

bool ExampleRenderingClient::Update()
{
	return m_Impl->Update();
}

bool ExampleRenderingClient::PrepareCommands()
{
	return m_Impl->PrepareCommands();
}

bool ExampleRenderingClient::ExecuteCommands(IRenderingConfig* renderingConfig)
{
	return m_Impl->ExecuteCommands(renderingConfig);
}

bool ExampleRenderingClient::FinalizeGPUResults()
{
	return m_Impl->FinalizeGPUResults();
}

bool ExampleRenderingClient::Terminate()
{
	if (m_Impl->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

std::vector<IRenderPass*> ExampleRenderingClient::GetDispatchedPasses() const
{
	// Order mirrors ExampleRenderingClientImpl::PrepareCommands. Both the
	// rasterizer fork and the GPU-path-tracer fork are listed because the
	// bypass flag on each pass persists across the active toggle — the editor
	// inspector wants to reach every pass the client owns. One-shot passes
	// (BRDFLUT*) are included for the same reason.
	std::vector<IRenderPass*> l_passes;
	l_passes.reserve(32);

	l_passes.push_back(&GPUPathTracerPass::Get());
	if constexpr (Inno::PTHashGridCache::ENABLED)
	{
		l_passes.push_back(&PTHashGridCachePurgeTilesPass::Get());
		l_passes.push_back(&PTHashGridCacheUpdateTilesPass::Get());
		l_passes.push_back(&PTHashGridCacheMipCascadeBuildPass::Get());
	}

	l_passes.push_back(&BRDFLUTPass::Get());
	l_passes.push_back(&BRDFLUTMSPass::Get());

	l_passes.push_back(&SunShadowRTPass::Get());

	l_passes.push_back(&OpaqueCullingPass::Get());
	l_passes.push_back(&OpaquePass::Get());

	l_passes.push_back(&RadianceCacheReprojectionPass::Get());
	l_passes.push_back(&RadianceCacheRaytracingPass::Get());
	l_passes.push_back(&RadianceCacheFilterHorizontalPass::Get());
	l_passes.push_back(&RadianceCacheFilterVerticalPass::Get());
	l_passes.push_back(&RadianceCacheIntegrationPass::Get());
	l_passes.push_back(&GIDenoisePass::Get());
	l_passes.push_back(&GIFilterHorizontalPass::Get());
	l_passes.push_back(&GIFilterVerticalPass::Get());

	l_passes.push_back(&SSAOPass::Get());

	l_passes.push_back(&TiledFrustumGenerationPass::Get());
	l_passes.push_back(&LightCullingPass::Get());
	l_passes.push_back(&LightPass::Get());
	l_passes.push_back(&SkyPass::Get());

	l_passes.push_back(&PreTAAPass::Get());
	l_passes.push_back(&TAAPass::Get());

	l_passes.push_back(&LuminanceHistogramPass::Get());
	l_passes.push_back(&LuminanceAveragePass::Get());
	l_passes.push_back(&FinalBlendPass::Get());

	return l_passes;
}

ObjectStatus ExampleRenderingClient::GetStatus()
{
	return m_Impl->GetStatus();
}
#include "ExampleRenderingClient_Internal.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/TextureResourceService.h"

#include "../../Engine/Engine.h"

#include <cstdlib>

using namespace Inno;

namespace Inno
{
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

		// Resolve audited outputs from the graph (the passes are JSON nodes now).
		auto l_graph = g_Engine->Get<RenderGraphService>();
		auto NodeRP = [&](const char* n) -> RenderPassComponent* { auto l_n = l_graph->FindNode(n); return l_n ? l_n->m_RenderPass : nullptr; };
		auto Res = [&](const char* n) { return static_cast<TextureComponent*>(l_graph->GetResource(n)); };
		Dump("audit_01_BRDFLUTPass.hdr",  NodeRP("BRDFLUTPass"),   Res("BRDF LUT"));
		Dump("audit_02_BRDFLUTMSPass.hdr", NodeRP("BRDFLUTMSPass"), Res("BRDF MS LUT"));
		Dump("audit_05_SSAO.hdr",          NodeRP("SSAONoisePass"), Res("SSAO_Result"));
		Dump("audit_09_Sky.hdr",           NodeRP("SkyPass"),       Res("Sky Pass Result"));

		Log(Success, "AuditDump complete. Check Bin/*.hdr");
		std::exit(0);
	}

	void ExampleRenderingClientImpl::RegisterAuditCallback()
	{
		if (!g_Engine->getInitConfig().isAudit)
			return;

		// SceneService keeps a raw pointer to the functor; store it as a
		// member so its lifetime matches the client (see EditorService.h
		// for the same convention). The callback only flips an atomic edge
		// flag — heavy work (frame counting, AuditDump invocation) runs
		// on the render thread in HandleAuditTrigger.
		m_AuditSceneLoadedCallback = [this]()
		{
			m_AuditSceneLoadEvent.store(true, std::memory_order_release);
		};
		g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&m_AuditSceneLoadedCallback);
	}

	void ExampleRenderingClientImpl::HandleAuditTrigger()
	{
		if (!g_Engine->getInitConfig().isAudit)
			return;

		// Two-phase event-driven trigger (replaces the prior
		// `s_AuditFrame == 30` absolute-frame fence which assumed scene-load
		// latency would stay within the first 5 frames):
		//   1. SceneService callback flips the cross-thread event flag after
		//      LoadSync completes (assets loaded, components initialised,
		//      GPU idle).
		//   2. Render thread observes the edge, resets the post-load
		//      counter, then dumps after K settle frames.
		// K=25 is an empirical settling budget covering GI cache fill,
		// TLAS build, and first-frame upload command-list drain — the
		// headroom the magic-30 fence bought minus the 5-frame pre-load
		// gap. Tighten later with capture-diff evidence.
		static constexpr uint32_t AuditPostLoadSettleFrames = 25;

		if (m_AuditSceneLoadEvent.exchange(false, std::memory_order_acquire))
		{
			m_AuditCountingStarted = true;
			m_AuditPostLoadFrameCount = 0;
			Log(Success, "Audit: post-load settle counter armed; will dump in ", AuditPostLoadSettleFrames, " frames.");
		}

		if (!m_AuditCountingStarted)
			return;

		++m_AuditPostLoadFrameCount;
		if (m_AuditPostLoadFrameCount >= AuditPostLoadSettleFrames)
			AuditDump();
	}
}

#include "ExampleRenderingClient_Internal.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SunShadowRTPass.h"
#include "OpaquePass.h"
#include "SSAOPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "TAAPass.h"
#include "FinalBlendPass.h"

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

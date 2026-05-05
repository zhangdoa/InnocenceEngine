#include "ExampleRenderingClient_Internal.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "FinalBlendPass.h"
#include "GPUPathTracerPass.h"
#include "TAAPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "PTHashGridCachePurgeTilesPass.h"
#include "PTHashGridCacheUpdateTilesPass.h"
#include "PTHashGridCacheMipCascadeBuildPass.h"
#include "HashGridCacheConstants.h"

#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/FrameManagementService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	#include "ExampleRenderingClient_Bypass.inl"

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
			ExecuteRasterizerPasses();

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

		HandleScreenCapture();
		HandleAutoCaptureTriggers();

		if (g_Engine->getInitConfig().isAudit)
		{
			static uint32_t s_AuditFrame = 0;
			if (++s_AuditFrame == 5)
				AuditDump();
		}

		return true;
	}
}

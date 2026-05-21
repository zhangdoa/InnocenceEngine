#include "ExampleRenderingClient_Internal.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "FinalBlendPass.h"
#include "PTPass.h"
#include "TAAPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "PTHashGridCachePurgeTilesPass.h"
#include "PTHashGridCacheUpdateTilesPass.h"
#include "PTHashGridCacheMipCascadeBuildPass.h"
#include "PTNRDFormatConvertPass.h"
#include "PTNRDDenoisePass.h"
#include "PTNRDCompositionPass.h"
#include "HashGridCacheConstants.h"
#include "NRDConstants.h"

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

		// PurgeTiles → UpdateTiles → MipCascadeBuild → PT chain;
		// per-block Waits enforce ordering. All four on Compute queue
		// (same-queue Signal/Wait, no graphics-side fence). The PT Wait
		// on MipCascadeBuild is reserved for future mip-aware Site-3
		// reads; today only mip 0 is consumed. Toggle-off elides the
		// block (passes stay Terminated, gates short-circuit).
		if constexpr (Inno::PTHashGridCache::ENABLED)
		{
			if (m_PTActive
				&& PTHashGridCachePurgeTilesPass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(PTHashGridCachePurgeTilesPass::Get()))
			{
				auto l_renderPass = PTHashGridCachePurgeTilesPass::Get().GetRenderPassComp();
				auto l_computeCL  = PTHashGridCachePurgeTilesPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
			}

			if (m_PTActive
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

			if (m_PTActive
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

		if (m_PTActive && PTPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(PTPass::Get()))
		{
			auto l_renderPass = PTPass::Get().GetRenderPassComp();

			// Graphics CL: transition accumulation buffer to UAV
			auto l_graphicsCL = PTPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCL, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Compute CL: ray tracing dispatch (also transitions AccumBuffer to ReadOnly at end).
			// Wait on MipCascadeBuild — last link in the cache chain — so this
			// frame's reads see both the resolved running mean (mip 0) and
			// the freshly-aggregated coarser cells (mips 1-3, currently
			// unread; reserved for a future mip-aware Site-3 lookup). The
			// same-queue Signal/Wait chain transitively covers the upstream
			// PurgeTiles + UpdateTiles waits.
			if constexpr (Inno::PTHashGridCache::ENABLED)
				WaitIfActive(PTHashGridCacheMipCascadeBuildPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);
			auto l_computeCL = PTPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		// PTNRDFormatConvert (TASK-77.4 CL-2). Runs on the Compute queue
		// after the path tracer finishes; the Graphics CL transition pre-
		// pass and the Compute dispatch each get their own Execute / Signal
		// pair. Toggle-OFF (BUILD_WITH_NRD=OFF): pass stays Terminated, no
		// dispatch — the entire block elides at compile time.
		if constexpr (Inno::NRD::ENABLED)
		{
			if (m_PTActive
				&& PTNRDFormatConvertPass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(PTNRDFormatConvertPass::Get()))
			{
				auto l_renderPass = PTNRDFormatConvertPass::Get().GetRenderPassComp();

				// Graphics CL: transition the five output UAVs into
				// the states the compute kernel expects. Required
				// because tracked state may include
				// PIXEL_SHADER_RESOURCE (set by swap chain
				// presentation), invalid on a compute command list.
				auto l_graphicsCL = PTNRDFormatConvertPass::Get().GetCommandListComp(GPUEngineType::Graphics);
				l_hwService->Execute(l_graphicsCL, GPUEngineType::Graphics);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
				l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

				// Wait on the path tracer — its compute dispatch wrote
				// the GBuffer + per-lobe radiance UAVs we read as SRVs.
				WaitIfActive(PTPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_computeCL = PTNRDFormatConvertPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
			}

			// PTNRDDenoise (TASK-77.4 CL-3). Compute-queue only — the
			// adapter records its NRD compute dispatches into the pass's
			// Compute CL via a raw ID3D12GraphicsCommandList* pulled out
			// by DX12Helper::AsDX12CommandList. No graphics-queue work
			// (the adapter manages its own resource transitions in-CL),
			// so no graphics Execute / Signal here. Wait on
			// PTNRDFormatConvertPass — its 5 output UAVs are this
			// pass's SRV inputs (already in NON_PIXEL_SHADER_RESOURCE
			// state from the format-convert exit transition).
			if (m_PTActive
				&& PTNRDDenoisePass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(PTNRDDenoisePass::Get()))
			{
				WaitIfActive(PTNRDFormatConvertPass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_renderPass = PTNRDDenoisePass::Get().GetRenderPassComp();
				auto l_computeCL  = PTNRDDenoisePass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
			}

			// PTNRDComposition (TASK-77.4 CL-3). Standard engine pass:
			// graphics CL transitions the output UAV from cross-frame
			// ReadOnly to ReadWrite, compute CL runs the unpack +
			// re-modulation kernel that produces the tonemap input.
			// Same pattern as PTNRDFormatConvertPass above. Wait on
			// PTNRDDenoise — its OUT_DIFF / OUT_SPEC borrowed shells
			// are this pass's SRV inputs (transitioned to
			// NON_PIXEL_SHADER_RESOURCE at the adapter's dispatch tail).
			if (m_PTActive
				&& PTNRDCompositionPass::Get().GetStatus() == ObjectStatus::Activated
				&& !IsBypassed(PTNRDCompositionPass::Get()))
			{
				auto l_renderPass = PTNRDCompositionPass::Get().GetRenderPassComp();

				auto l_graphicsCL = PTNRDCompositionPass::Get().GetCommandListComp(GPUEngineType::Graphics);
				l_hwService->Execute(l_graphicsCL, GPUEngineType::Graphics);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
				l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

				WaitIfActive(PTNRDDenoisePass::Get(), GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_computeCL = PTNRDCompositionPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
				l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
			}
		}

		if (!m_PTActive)
			ExecuteRasterizerPasses();

		if (LuminanceHistogramPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(LuminanceHistogramPass::Get()))
		{
			if (m_PTActive)
			{
				WaitIfActive(PTPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
				// Under NRD-ENABLED the histogram source is the
				// composition output, not the PT result. WaitIfActive
				// is a no-op when the pass is unactivated / bypassed,
				// so the PT-only wait above stays correct on the OFF
				// path; NRD-ON adds the second wait without re-shaping
				// the OFF code.
				if constexpr (Inno::NRD::ENABLED)
					WaitIfActive(PTNRDCompositionPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			}
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
			if (m_PTActive)
			{
				WaitIfActive(PTPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
				if constexpr (Inno::NRD::ENABLED)
					WaitIfActive(PTNRDCompositionPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);
			}
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
		HandleAuditTrigger();

		return true;
	}
}

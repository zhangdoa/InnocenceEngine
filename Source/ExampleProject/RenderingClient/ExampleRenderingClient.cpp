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
#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
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
		if constexpr (Inno::NRD::ENABLED)
		{
			PTNRDFormatConvertPass::Get().Initialize();
			PTNRDDenoisePass::Get().Initialize();
			PTNRDCompositionPass::Get().Initialize();
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
			if constexpr (Inno::NRD::ENABLED)
			{
				PTNRDFormatConvertPass::Get().Update();
				PTNRDDenoisePass::Get().Update();
				PTNRDCompositionPass::Get().Update();
			}
		}

		return true;
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

		// NRD chain (TASK-77.4 CL-2 + CL-3) terminates before any consumer
		// pass — Composition reads NRD outputs, Denoise owns the adapter
		// (which holds raw ID3D12Resource* allocations and must release them
		// before the device dies). Order: composition → denoise → format-
		// convert (reverse of frame execution: consumer → producer).
		if constexpr (Inno::NRD::ENABLED)
		{
			PTNRDCompositionPass::Get().Terminate();
			PTNRDDenoisePass::Get().Terminate();
			PTNRDFormatConvertPass::Get().Terminate();
		}

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
	if constexpr (Inno::NRD::ENABLED)
	{
		l_passes.push_back(&PTNRDFormatConvertPass::Get());
		l_passes.push_back(&PTNRDDenoisePass::Get());
		l_passes.push_back(&PTNRDCompositionPass::Get());
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

#include "ExampleRenderingClient_Internal.h"
#include "OpaquePass.h"
#include "RadianceCacheReprojectionPass.h"
#include "RadianceCacheRaytracingPass.h"
#include "RadianceCacheFilterHorizontalPass.h"
#include "RadianceCacheFilterVerticalPass.h"
#include "RadianceCacheIntegrationPass.h"
#include "GIDenoisePass.h"
#include "GIFilterHorizontalPass.h"
#include "GIFilterVerticalPass.h"

#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	#include "ExampleRenderingClient_Bypass.inl"

	void ExampleRenderingClientImpl::ExecuteGIPasses()
	{
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

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
	}
}

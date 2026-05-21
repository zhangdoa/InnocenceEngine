#include "ExampleRenderingClient_Internal.h"
#include "OpaquePass.h"
#include "SSRCReprojectionPass.h"
#include "SSRCRaytracingPass.h"
#include "SSRCFilterHorizontalPass.h"
#include "SSRCFilterVerticalPass.h"
#include "SSRCIntegrationPass.h"
#include "SSRCTemporalPass.h"
#include "SSRCSpatialHorizontalPass.h"
#include "SSRCSpatialVerticalPass.h"

#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	#include "ExampleRenderingClient_Bypass.inl"

	void ExampleRenderingClientImpl::ExecuteGIPasses()
	{
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

		if (SSRCReprojectionPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCReprojectionPass::Get()))
		{
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = SSRCReprojectionPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSRCReprojectionPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSRCReprojectionPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSRCRaytracingPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCRaytracingPass::Get()))
		{
			WaitIfActive(SSRCReprojectionPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = SSRCRaytracingPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSRCRaytracingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			auto l_computeCommandList = SSRCRaytracingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSRCFilterHorizontalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCFilterHorizontalPass::Get()))
		{
			WaitIfActive(SSRCRaytracingPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = SSRCFilterHorizontalPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSRCFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSRCFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSRCFilterVerticalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCFilterVerticalPass::Get()))
		{
			WaitIfActive(SSRCFilterHorizontalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = SSRCFilterVerticalPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSRCFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSRCFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSRCIntegrationPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCIntegrationPass::Get()))
		{
			WaitIfActive(SSRCFilterVerticalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = SSRCIntegrationPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSRCIntegrationPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSRCIntegrationPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSRCTemporalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCTemporalPass::Get()))
		{
			WaitIfActive(OpaquePass::Get(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			WaitIfActive(SSRCIntegrationPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = SSRCTemporalPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSRCTemporalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSRCTemporalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSRCSpatialHorizontalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCSpatialHorizontalPass::Get()))
		{
			WaitIfActive(SSRCTemporalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = SSRCSpatialHorizontalPass::Get().GetRenderPassComp();
			l_hwService->Execute(SSRCSpatialHorizontalPass::Get().GetCommandListComp(GPUEngineType::Graphics), GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			l_hwService->Execute(SSRCSpatialHorizontalPass::Get().GetCommandListComp(GPUEngineType::Compute), GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSRCSpatialVerticalPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(SSRCSpatialVerticalPass::Get()))
		{
			WaitIfActive(SSRCSpatialHorizontalPass::Get(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = SSRCSpatialVerticalPass::Get().GetRenderPassComp();
			l_hwService->Execute(SSRCSpatialVerticalPass::Get().GetCommandListComp(GPUEngineType::Graphics), GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			l_hwService->Execute(SSRCSpatialVerticalPass::Get().GetCommandListComp(GPUEngineType::Compute), GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}
	}
}

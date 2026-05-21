#include "ExampleRenderingClient_Internal.h"
#include "SunShadowRTPass.h"
#include "OpaqueCullingPass.h"
#include "OpaquePass.h"
#include "SSAOPass.h"
#include "GIDenoisePass.h"
#include "GIFilterVerticalPass.h"
#include "TiledFrustumGenerationPass.h"
#include "LightCullingPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TAAPass.h"

#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	#include "ExampleRenderingClient_Bypass.inl"

	void ExampleRenderingClientImpl::ExecuteRasterizerPasses()
	{
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

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

		ExecuteGIPasses();

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

			auto l_graphicsCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LightPass::Get().GetStatus() == ObjectStatus::Activated && !IsBypassed(LightPass::Get()))
		{
			// Suspended SunShadowRT (e.g. early frames before TLAS build) → LightPass binds
			// nullptr at slot t13 and the sun is treated as fully shadowed for that frame.
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
	}
}

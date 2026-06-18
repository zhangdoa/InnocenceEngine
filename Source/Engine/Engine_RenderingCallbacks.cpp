#include "Engine_Internal.h"
#include "Common/LogService.h"
#include "Services/ConfigurationService.h"
#include "Services/EntityRegistry.h"
#include "Services/TransformService.h"
#include "RenderGraph/RenderGraphService.h"
#include "Services/ScreenCaptureService.h"
#include "Services/AuditDumpService.h"
#include "Services/LightSimulationService.h"
#include "Services/CameraService.h"
#include "Services/SceneService.h"
#include "Services/PerFrameDataService.h"
#include "Services/LightDataService.h"
#include "Services/DrawCallService.h"
#include "Services/AnimationDrawCallService.h"
#include "Services/BillboardDrawCallService.h"
#include "Services/DebugDrawCallService.h"
#include "Services/AnimationResourceService.h"
#include "Services/AnimationSimulationService.h"
#include "Services/GraphicsHardwareService.h"
#include "Services/FrameManagementService.h"

using namespace Inno;

void Engine::WireRenderingCallbacks()
{
	Get<FrameManagementService>()->SetUploadHeapPreparationCallback([&]()
		{
			SystemUpdate(SceneService);

			if (Get<SceneService>()->IsLoading())
				return true;

			if (m_pImpl->m_LogicClient)
				m_pImpl->m_LogicClient->Update();

			Get<CameraService>()->Update();
			Get<LightSimulationService>()->Update();
			SystemUpdate(EntityRegistry);
			Get<TransformService>()->Update();
			Get<PerFrameDataService>()->Update();
			Get<LightDataService>()->Update();
			Get<DrawCallService>()->Update();
			Get<AnimationDrawCallService>()->Update();
			Get<BillboardDrawCallService>()->Update();
			Get<DebugDrawCallService>()->Update();
			Get<AnimationSimulationService>()->Update();
			Get<AnimationResourceService>()->Update();
			if (m_pImpl->m_RenderingClient)
				m_pImpl->m_RenderingClient->Update();

			return true;
		});

	Get<FrameManagementService>()->SetCommandPreparationCallback([&]()
		{
			return true;
		});

	Get<FrameManagementService>()->SetCommandExecutionCallback([&]()
		{
			if (Get<SceneService>()->IsLoading())
				return true;

			Get<RenderGraphService>()->Render();
			Get<ScreenCaptureService>()->Update();
			Get<AuditDumpService>()->Update();

			return true;
		});

	const int l_captureFrame = g_Engine->Get<ConfigurationService>()->GetCaptureFrame();
	if (l_captureFrame >= 0)
	{
		// captureFrame is a session-relative index (frames since steady state),
		// so the capture lands on a converged frame regardless of load time.
		Get<FrameManagementService>()->SetPreFrameCallback([this, l_captureFrame](uint32_t)
			{
				auto* l_fm = Get<FrameManagementService>();
				if (l_fm->HasReachedSteadyState()
					&& l_fm->GetSteadyStateRelativeFrameCount() == static_cast<uint32_t>(l_captureFrame))
					Get<GraphicsHardwareService>()->BeginCapture();
			});

		Get<FrameManagementService>()->SetPostFrameCallback([this, l_captureFrame](uint32_t)
			{
				auto* l_fm = Get<FrameManagementService>();
				if (!l_fm->HasReachedSteadyState()
					|| l_fm->GetSteadyStateRelativeFrameCount() != static_cast<uint32_t>(l_captureFrame))
					return;

				// Drain all queues so the capture boundary encloses a complete
				// frame; EndFrameCapture would otherwise see mid-flight state.
				auto* l_hw = Get<GraphicsHardwareService>();
				l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
				l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Compute),  GPUEngineType::Compute);
				l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Copy),     GPUEngineType::Copy);
				l_hw->EndCapture();
			});
	}
}

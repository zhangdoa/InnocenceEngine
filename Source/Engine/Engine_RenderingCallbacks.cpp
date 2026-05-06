#include "Engine_Internal.h"
#include "Common/LogService.h"
#include "Services/EntityRegistry.h"
#include "Services/TransformService.h"
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
			if (Get<SceneService>()->IsLoading())
				return true;

			if (m_pImpl->m_RenderingClient) {
				m_pImpl->m_RenderingClient->PrepareCommands();
			}
			return true;
		});

	Get<FrameManagementService>()->SetCommandExecutionCallback([&]()
		{
			if (Get<SceneService>()->IsLoading())
				return true;

			if (m_pImpl->m_RenderingClient) {
				m_pImpl->m_RenderingClient->ExecuteCommands();
			}
			return true;
		});

	// RenderDoc / PIX capture trigger. Lives here rather than inside
	// FrameManagementService::Update because the frame manager owns frame
	// pacing, not debug-tool triggers. -capture_frame N fires a single
	// frame capture at frame N (0-indexed, FrameCountSinceLaunch).
	const int l_captureFrame = m_pImpl->m_initConfig.captureFrame;
	if (l_captureFrame >= 0)
	{
		Get<FrameManagementService>()->SetPreFrameCallback([this, l_captureFrame](uint32_t frameCount)
			{
				if (frameCount == static_cast<uint32_t>(l_captureFrame))
					Get<GraphicsHardwareService>()->BeginCapture();
			});

		Get<FrameManagementService>()->SetPostFrameCallback([this, l_captureFrame](uint32_t frameCount)
			{
				if (frameCount != static_cast<uint32_t>(l_captureFrame))
					return;

				// Drain all queued GPU work so the capture boundary encloses
				// a complete frame — RenderDoc's EndFrameCapture otherwise
				// sees a mid-flight state and records no useful frame.
				auto* l_fm = Get<FrameManagementService>();
				auto* l_hw = Get<GraphicsHardwareService>();
				l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
				l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Compute),  GPUEngineType::Compute);
				l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Copy),     GPUEngineType::Copy);
				(void)l_fm;
				l_hw->EndCapture();
			});
	}
}

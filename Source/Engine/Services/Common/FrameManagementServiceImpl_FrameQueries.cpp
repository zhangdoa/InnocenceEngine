#include "../FrameManagementService.h"
#include "../GPUBufferResourceService.h"
#include "../MeshResourceService.h"
#include "../SceneService.h"
#include "../ConfigurationService.h"
#include "../../RenderGraph/RenderGraphService.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

#include "../../Engine.h"

using namespace Inno;

uint32_t FrameManagementService::GetCurrentFrame()
{
	return m_CurrentFrame;
}

uint32_t FrameManagementService::GetPreviousFrame()
{
	return m_CurrentFrame == 0 ? m_swapChainImageCount - 1 : m_CurrentFrame - 1;
}

uint32_t FrameManagementService::GetNextFrame()
{
	return m_CurrentFrame == m_swapChainImageCount - 1 ? 0 : m_CurrentFrame + 1;
}

uint32_t FrameManagementService::GetSwapChainImageCount()
{
	return m_swapChainImageCount;
}

uint32_t FrameManagementService::GetFrameCountSinceLaunch()
{
	return m_FrameCountSinceLaunch;
}

bool FrameManagementService::IsSteadyState()
{
	// K=3 rides past single-frame TLAS-rebuild gaps observed during the
	// deferred-mesh-activation drain. K=2 fires inside the drain window.
	static constexpr uint32_t TLASStabilityWindowFrames = 3;
	static constexpr uint32_t SteadyStateTimeoutFrames = 120;

	auto l_meshService = g_Engine->Get<MeshResourceService>();
	auto l_gpuBufferService = g_Engine->Get<GPUBufferResourceService>();
	auto l_sceneService = g_Engine->Get<SceneService>();

	const size_t l_currentInstanceCount = l_gpuBufferService->GetRaytracingInstanceCount();
	if (l_currentInstanceCount != m_LastObservedInstanceCount)
	{
		const size_t l_previousInstanceCount = m_LastObservedInstanceCount;
		m_LastObservedInstanceCount = l_currentInstanceCount;
		m_TLASStableFrameCount = 0;

		if (m_SteadyStateMarkerLogged)
		{
			Log(Verbose, "Auto-test: steady state lost at frame=", m_FrameCountSinceLaunch.load(),
				" (instanceCount changed ", l_previousInstanceCount, "->", l_currentInstanceCount, ")");
		}
	}
	else
	{
		++m_TLASStableFrameCount;
	}

	const bool l_deferredQueueEmpty = l_meshService->IsDeferredQueueEmpty();
	const bool l_tlasStable = m_TLASStableFrameCount >= TLASStabilityWindowFrames;
	const bool l_notLoading = !l_sceneService->IsLoading();
	const bool l_steadyState = l_deferredQueueEmpty && l_tlasStable && l_notLoading;

	// The "Auto-test: steady state reached at frame=" string is grepped by
	// external capture-harness tooling; do not reword.
	if (l_steadyState && !m_SteadyStateMarkerLogged)
	{
		m_SteadyStateMarkerLogged = true;
		if (m_FirstSteadyStateFrame == UINT32_MAX)
			m_FirstSteadyStateFrame = m_FrameCountSinceLaunch.load();
		Log(Verbose, "Auto-test: steady state reached at frame=", m_FrameCountSinceLaunch.load(),
			" deferredQueueEmpty=", l_deferredQueueEmpty,
			" tlasStableFrames=", m_TLASStableFrameCount,
			" instanceCount=", l_currentInstanceCount,
			" isLoading=", !l_notLoading);
	}

	if (!m_SteadyStateMarkerLogged
		&& !m_SteadyStateTimeoutLogged
		&& m_FrameCountSinceLaunch.load() >= SteadyStateTimeoutFrames)
	{
		m_SteadyStateTimeoutLogged = true;
		// Watchdog fallback: anchor the session clock here so frame-indexed
		// triggers and the totalFrames budget still fire (degraded determinism,
		// already warned) instead of hanging forever waiting for true steady.
		if (m_FirstSteadyStateFrame == UINT32_MAX)
			m_FirstSteadyStateFrame = m_FrameCountSinceLaunch.load();
		Log(Warning, "Auto-test: steady state NOT reached within ", SteadyStateTimeoutFrames,
			" frames — capture determinism cannot be guaranteed."
			" deferredQueueEmpty=", l_deferredQueueEmpty,
			" tlasStableFrames=", m_TLASStableFrameCount,
			" instanceCount=", l_currentInstanceCount,
			" isLoading=", !l_notLoading);
	}

	return l_steadyState;
}

uint32_t FrameManagementService::GetSteadyStateRelativeFrameCount() const
{
	if (m_FirstSteadyStateFrame == UINT32_MAX)
		return 0u;
	const uint32_t l_now = m_FrameCountSinceLaunch.load();
	return l_now >= m_FirstSteadyStateFrame ? (l_now - m_FirstSteadyStateFrame) : 0u;
}

void FrameManagementService::EvaluateFrameBudget()
{
	const int l_totalFrames = g_Engine->Get<ConfigurationService>()->GetTotalFrames();
	if (l_totalFrames <= 0)
		return;

	// Render session-frames [0..totalFrames] inclusive — a trigger registered at
	// frame==totalFrames still fires this turn — then request a clean shutdown.
	if (GetSteadyStateRelativeFrameCount() > static_cast<uint32_t>(l_totalFrames))
		g_Engine->RequestShutdown();
}

void FrameManagementService::SetUploadHeapPreparationCallback(std::function<bool()>&& callback)
{
	m_UploadHeapPreparationCallback = callback;
}

void FrameManagementService::SetCommandPreparationCallback(std::function<bool()>&& callback)
{
	m_CommandPreparationCallback = callback;
}

void FrameManagementService::SetCommandExecutionCallback(std::function<bool()>&& callback)
{
	m_CommandExecutionCallback = callback;
}

void FrameManagementService::SetPreFrameCallback(std::function<void(uint32_t)>&& callback)
{
	m_PreFrameCallback = std::move(callback);
}

void FrameManagementService::SetPostFrameCallback(std::function<void(uint32_t)>&& callback)
{
	m_PostFrameCallback = std::move(callback);
}

RenderPassComponent* FrameManagementService::GetSwapChainRenderPassComponent()
{
	return m_SwapChainRenderPassComp;
}

GPUResourceComponent* FrameManagementService::GetUserPipelineOutput()
{
	const auto& l_canvasName = g_Engine->Get<ConfigurationService>()->GetCanvasResourceName();
	return g_Engine->Get<RenderGraphService>()->GetResource(l_canvasName);
}

ISemaphore* FrameManagementService::GetGlobalSemaphore()
{
	return m_GlobalSemaphore;
}

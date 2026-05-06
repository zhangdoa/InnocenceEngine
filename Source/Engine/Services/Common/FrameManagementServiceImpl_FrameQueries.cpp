#include "../FrameManagementService.h"
#include "../GPUBufferResourceService.h"
#include "../MeshResourceService.h"
#include "../SceneService.h"

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

// TASK-213 CL A: scene-readiness predicate. See header for contract.
// Evaluated once per Update() so the rolling-K stability counter advances
// frame-by-frame; the result is recomputed on demand without caching, so
// callers always observe the current frame's state.
bool FrameManagementService::IsSteadyState()
{
	// K=3 frames of TLAS instance-count stability. Log evidence (TASK-210
	// post-closure follow-up Ext.1, captured under
	// Build/captures/TASK-77.1-D1-reversal/D/toggle0/gisponza/engine.log)
	// shows GISponza rebuilds at frame=0/1/2/16/30 — successive single-frame
	// gaps. K=2 would have fired between frame=2 and frame=16; K=3 rides
	// past the early bursts and only goes true once the deferred-init drain
	// has actually settled.
	static constexpr uint32_t TLASStabilityWindowFrames = 3;
	// 120 frames at 60 Hz = 2 seconds. Long enough for any current scene's
	// deferred-init drain to complete (GISponza is the slowest at frame=30);
	// short enough that capture infra notices a misbehaving run before the
	// -total_frames cap silently terminates.
	static constexpr uint32_t SteadyStateTimeoutFrames = 120;

	auto l_meshService = g_Engine->Get<MeshResourceService>();
	auto l_gpuBufferService = g_Engine->Get<GPUBufferResourceService>();
	auto l_sceneService = g_Engine->Get<SceneService>();

	// Update the rolling TLAS-stability counter from the current observation.
	// Reset on count change; increment when count is unchanged.
	const size_t l_currentInstanceCount = l_gpuBufferService->GetRaytracingInstanceCount();
	if (l_currentInstanceCount != m_LastObservedInstanceCount)
	{
		const size_t l_previousInstanceCount = m_LastObservedInstanceCount;
		m_LastObservedInstanceCount = l_currentInstanceCount;
		m_TLASStableFrameCount = 0;

		// Flap-back log: if the steady-state marker has already latched and
		// the predicate is now losing its TLAS-stable state, emit a Verbose
		// line so CL B's consumer sees the full flap timeline. The marker
		// itself is one-shot per session per scene (existing behavior); this
		// log does NOT clear m_SteadyStateMarkerLogged.
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

	// Log-once marker on first transition to true. Harness-grep target for
	// CL D ("Auto-test: steady state reached at frame=N") — the wording is
	// load-bearing for the script that will run later in the chain; do not
	// reword without updating CL D.
	if (l_steadyState && !m_SteadyStateMarkerLogged)
	{
		m_SteadyStateMarkerLogged = true;
		// TASK-213 CL B: snapshot the absolute frame count at the latch instant
		// so GetSteadyStateRelativeFrameCount() can compute the steady-state-
		// relative offset. Set once, never reset (flap-back leaves this stable
		// per CL A's reviewer carry-forward — the running mean must not be
		// invalidated by a late TLAS rebuild).
		m_FirstSteadyStateFrame = m_FrameCountSinceLaunch.load();
		Log(Verbose, "Auto-test: steady state reached at frame=", m_FrameCountSinceLaunch.load(),
			" deferredQueueEmpty=", l_deferredQueueEmpty,
			" tlasStableFrames=", m_TLASStableFrameCount,
			" instanceCount=", l_currentInstanceCount,
			" isLoading=", !l_notLoading);
	}

	// 120-frame timeout watchdog (R2). Loud Warning so a future capture run
	// that never reaches steady state is obvious rather than silent.
	if (!m_SteadyStateMarkerLogged
		&& !m_SteadyStateTimeoutLogged
		&& m_FrameCountSinceLaunch.load() >= SteadyStateTimeoutFrames)
	{
		m_SteadyStateTimeoutLogged = true;
		Log(Warning, "Auto-test: steady state NOT reached within ", SteadyStateTimeoutFrames,
			" frames — capture determinism cannot be guaranteed."
			" deferredQueueEmpty=", l_deferredQueueEmpty,
			" tlasStableFrames=", m_TLASStableFrameCount,
			" instanceCount=", l_currentInstanceCount,
			" isLoading=", !l_notLoading);
	}

	return l_steadyState;
}

// TASK-213 CL B: steady-state-relative frame count for capture-mode
// determinism. Returns 0 until the steady-state marker first latches, then
// returns m_FrameCountSinceLaunch - m_FirstSteadyStateFrame. Both the dump-
// frame filename (`gpu_output_NNNN.png`) and the PT-RNG seed in capture mode
// read from this so the value at the dump frame is signal-driven (not
// load-frame-count driven) and reproducible across same-binary launches.
uint32_t FrameManagementService::GetSteadyStateRelativeFrameCount() const
{
	if (m_FirstSteadyStateFrame == UINT32_MAX)
		return 0u;
	const uint32_t l_now = m_FrameCountSinceLaunch.load();
	return l_now >= m_FirstSteadyStateFrame ? (l_now - m_FirstSteadyStateFrame) : 0u;
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

bool FrameManagementService::SetUserPipelineOutput(std::function<GPUResourceComponent* ()>&& getUserPipelineOutputFunc)
{
	m_GetUserPipelineOutputFunc = getUserPipelineOutputFunc;
	return true;
}

GPUResourceComponent* FrameManagementService::GetUserPipelineOutput()
{
	return m_GetUserPipelineOutputFunc();
}

ISemaphore* FrameManagementService::GetGlobalSemaphore()
{
	return m_GlobalSemaphore;
}

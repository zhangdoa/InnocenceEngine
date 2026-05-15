#include "Engine_Internal.h"
#include "Common/LogService.h"
#include "Common/TaskScheduler.h"
#include "Services/EntityRegistry.h"
#include "Services/TransformService.h"
#include "Services/LightSimulationService.h"
#include "Services/CameraService.h"
#include "Services/SceneService.h"
#include "Services/AssetService.h"
#include "Services/PhysicsSimulationService.h"
#include "Services/HIDService.h"
#include "Services/TemplateAssetService.h"
#include "Services/PerFrameDataService.h"
#include "Services/LightDataService.h"
#include "Services/DrawCallService.h"
#include "Services/AnimationDrawCallService.h"
#include "Services/BillboardDrawCallService.h"
#include "Services/DebugDrawCallService.h"
#include "Services/AnimationResourceService.h"
#include "Services/AnimationSimulationService.h"
#include "Services/GUIService.h"
#include "Services/EditorService.h"
#include "Services/FrameManagementService.h"
#include "Services/CommandListResourceService.h"
#include "Services/SamplerResourceService.h"
#include "Services/ShaderProgramResourceService.h"
#include "Services/TextureResourceService.h"
#include "Services/GPUBufferResourceService.h"
#include "Services/MeshResourceService.h"
#include "Services/MaterialResourceService.h"
#include "Services/RenderPassResourceService.h"

using namespace Inno;

bool Engine::Terminate()
{
	if (m_pImpl->m_RenderingExecutionTask) {
		m_pImpl->m_RenderingExecutionTask->Wait();
		m_pImpl->m_RenderingExecutionTask->Deactivate();
	}

	// Drain the GPU before destroying resources — the last rendered frame's
	// commands may still be in-flight since no subsequent BeginFrame waited.
	if (!m_pImpl->m_initConfig.isHeadless) {
		Get<FrameManagementService>()->WaitForGPUIdle();
	}

	// GPU-alive finalization must run before LogicClient::Terminate; the next
	// phase may stall the GPU long enough to trip TDR.
	if (!m_pImpl->m_initConfig.isHeadless && m_pImpl->m_RenderingClient) {
		if (!m_pImpl->m_RenderingClient->FinalizeGPUResults())
			Log(Warning, "RenderingClient::FinalizeGPUResults reported failure; continuing shutdown.");
	}

	// Bake mode skips LogicClient::Terminate — the client was never Setup/Initialize'd.
	if (m_pImpl->m_LogicClient && !m_pImpl->m_initConfig.isBakeMode) {
		if (!m_pImpl->m_LogicClient->Terminate())
		{
			Log(Error, "Logic client can't be terminated!");
			return false;
		}
	}

	if (!m_pImpl->m_initConfig.isHeadless) {
		ITask::Desc taskDesc("Default Rendering Client Termination Task", ITask::Type::Once, 2);
		auto l_ExampleRenderingClientTerminationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (!m_pImpl->m_initConfig.isOffscreen)
				SystemTerm(GUIService);

			if (m_pImpl->m_RenderingClient && !m_pImpl->m_RenderingClient->Terminate())
			{
				Log(Error, "Rendering client can't be terminated!");
				return false;
			}
			return true;
			});
		l_ExampleRenderingClientTerminationTask->Activate();
		l_ExampleRenderingClientTerminationTask->Wait();

		SystemTerm(AnimationResourceService);
		SystemTerm(AnimationSimulationService);
		SystemTerm(DebugDrawCallService);
		SystemTerm(BillboardDrawCallService);
		SystemTerm(AnimationDrawCallService);
		SystemTerm(DrawCallService);
		SystemTerm(LightDataService);
		SystemTerm(PerFrameDataService);
		SystemTerm(TemplateAssetService);

		Get<FrameManagementService>()->Terminate();
		Get<RenderPassResourceService>()->Terminate();
		Get<MaterialResourceService>()->Terminate();
		Get<MeshResourceService>()->Terminate();
		Get<GPUBufferResourceService>()->Terminate();
		Get<TextureResourceService>()->Terminate();
		Get<ShaderProgramResourceService>()->Terminate();
		Get<SamplerResourceService>()->Terminate();
		Get<CommandListResourceService>()->Terminate();
	}

	SystemTerm(CameraService);
	SystemTerm(LightSimulationService);

	if (m_pImpl->m_initConfig.engineMode == EngineMode::Sidecar)
	{
		SystemTerm(EditorService);
	}

	SystemTerm(PhysicsSimulationService);
	SystemTerm(SceneService);
	SystemTerm(AssetService);
	SystemTerm(TransformService);

	SystemTerm(EntityRegistry);

	if (!m_pImpl->m_WindowSystem->Terminate())
	{
		Log(Error, "WindowSystem can't be terminated!");
		return false;
	}

	SystemTerm(HIDService);

	Get<TaskScheduler>()->Freeze();
	Get<TaskScheduler>()->Reset();

	m_pImpl->m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "Engine has been terminated.");

	return true;
}

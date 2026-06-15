#include "Engine_Internal.h"
#include "Common/LogService.h"
#include "Common/TaskScheduler.h"
#include "Services/ConfigurationService.h"
#include "Services/EntityRegistry.h"
#include "Services/DevToggleRegistry.h"
#include "Services/ScreenCaptureService.h"
#include "Services/AuditDumpService.h"
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
	auto* l_cfg = g_Engine->Get<ConfigurationService>();
	if (!l_cfg->IsHeadless()) {
		Get<FrameManagementService>()->WaitForGPUIdle();
	}

	// GPU-alive finalization: capture readback must run before LogicClient
	// shutdown — the next phase may stall the GPU long enough to trip TDR.
	// Registered toggle callbacks capture client state; clear the registry
	// before clients start to die so an in-flight WS message can't deref it.
	if (!l_cfg->IsHeadless())
	{
		Get<ScreenCaptureService>()->TryWriteAutoCapture();
		DevToggleRegistry::Clear();
	}


	// Bake mode skips LogicClient::Terminate — the client was never Setup/Initialize'd.
	if (m_pImpl->m_LogicClient && !l_cfg->IsBakeMode()) {
		if (!m_pImpl->m_LogicClient->Terminate())
		{
			Log(Error, "Logic client can't be terminated!");
			return false;
		}
	}

	if (!l_cfg->IsHeadless()) {
		ITask::Desc taskDesc("Default Rendering Client Termination Task", ITask::Type::Once, 2);
		auto l_ExampleRenderingClientTerminationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (!l_cfg->IsOffscreen())
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

		SystemTerm(AuditDumpService);
		SystemTerm(ScreenCaptureService);
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

	if (l_cfg->GetEngineMode() == EngineMode::Sidecar)
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

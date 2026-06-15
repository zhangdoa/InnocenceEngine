#include "Engine_Internal.h"
#include "Common/Timer.h"
#include "Common/LogService.h"
#include "Common/TaskScheduler.h"
#include "Services/ConfigurationService.h"
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
#include "Services/ScreenCaptureService.h"
#include "Services/AuditDumpService.h"
#include "Services/GraphicsHardwareService.h"
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

bool Engine::Setup(void* appHook, void* extraHook, char* pScmdline,
	std::unique_ptr<IRenderingClient> renderingClient,
	std::unique_ptr<ILogicClient> logicClient)
{
	if (!CreateServices(appHook, extraHook, pScmdline))
		return false;

	auto* l_cfg = g_Engine->Get<ConfigurationService>();

	m_pImpl->m_RenderingClient = std::move(renderingClient);
	m_pImpl->m_LogicClient = std::move(logicClient);

	if (m_pImpl->m_LogicClient)
	{
		if (l_cfg->IsOffscreen())
		{
			m_pImpl->m_applicationName = "OffscreenEngine";
			Log(Success, "Offscreen mode: LogicClient and RenderingClient injected for testing.");
		}
		else
		{
			m_pImpl->m_applicationName = m_pImpl->m_LogicClient->GetApplicationName();
		}
	}
	else
	{
		m_pImpl->m_applicationName = "HeadlessEngine";
		Log(Success, "No clients injected: running headless.");
	}

	SystemSetup(HIDService);

	IWindowServiceConfig l_windowSystemConfig;
	l_windowSystemConfig.m_AppHook = appHook;
	l_windowSystemConfig.m_ExtraHook = extraHook;

	if (!m_pImpl->m_WindowSystem->Setup(&l_windowSystemConfig))
	{
		Log(Error, "Window System can't be setup!");
		return false;
	}

	SystemSetup(EntityRegistry);
	SystemSetup(TransformService);

	SystemSetup(AssetService);
	SystemSetup(SceneService);
	SystemSetup(PhysicsSimulationService);

	SystemSetup(LightSimulationService);
	SystemSetup(CameraService);

	if (l_cfg->GetEngineMode() == EngineMode::Sidecar)
	{
		SystemSetup(EditorService);
	}

	SystemSetup(TemplateAssetService);

	if (!l_cfg->IsHeadless())
	{
		if (!Get<CommandListResourceService>()->Setup())
		{
			Log(Error, "CommandListResourceService can't be setup!");
			return false;
		}
		if (!Get<SamplerResourceService>()->Setup())
		{
			Log(Error, "SamplerResourceService can't be setup!");
			return false;
		}
		if (!Get<ShaderProgramResourceService>()->Setup())
		{
			Log(Error, "ShaderProgramResourceService can't be setup!");
			return false;
		}
		if (!Get<TextureResourceService>()->Setup())
		{
			Log(Error, "TextureResourceService can't be setup!");
			return false;
		}
		if (!Get<GPUBufferResourceService>()->Setup())
		{
			Log(Error, "GPUBufferResourceService can't be setup!");
			return false;
		}
		if (!Get<MeshResourceService>()->Setup())
		{
			Log(Error, "MeshResourceService can't be setup!");
			return false;
		}
		if (!Get<MaterialResourceService>()->Setup())
		{
			Log(Error, "MaterialResourceService can't be setup!");
			return false;
		}
		if (!Get<RenderPassResourceService>()->Setup())
		{
			Log(Error, "RenderPassResourceService can't be setup!");
			return false;
		}

		if (!Get<GraphicsHardwareService>()->Setup())
		{
			Log(Error, "GraphicsHardwareService can't be setup!");
			return false;
		}

		if (!Get<FrameManagementService>()->Setup())
		{
			Log(Error, "FrameManagementService can't be setup!");
			return false;
		}
	}

	if (!l_cfg->IsHeadless()) {
		WireRenderingCallbacks();
	}

	if (!l_cfg->IsHeadless()) {
		SystemSetup(PerFrameDataService);
		SystemSetup(LightDataService);
		SystemSetup(DrawCallService);
		SystemSetup(AnimationDrawCallService);
		SystemSetup(BillboardDrawCallService);
		SystemSetup(DebugDrawCallService);
		SystemSetup(AnimationSimulationService);
		SystemSetup(AnimationResourceService);
		SystemSetup(ScreenCaptureService);
		SystemSetup(AuditDumpService);

		ITask::Desc taskDesc("Default Rendering Client Setup Task", ITask::Type::Once, 2);
		auto l_ExampleRenderingClientSetupTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (m_pImpl->m_RenderingClient) {
				if (!m_pImpl->m_RenderingClient->Setup())
				{
					Log(Error, "Rendering Client can't be setup!");
					return false;
				}
			}

			if (!l_cfg->IsOffscreen())
				SystemSetup(GUIService);

			return true;
			});

		l_ExampleRenderingClientSetupTask->Activate();
		l_ExampleRenderingClientSetupTask->Wait();
	}

	if (m_pImpl->m_LogicClient && !l_cfg->IsBakeMode()) {
		if (!m_pImpl->m_LogicClient->Setup())
		{
			Log(Error, "Logic Client can't be setup!");
			return false;
		}
	}

	if (!l_cfg->IsHeadless())
	{
		m_pImpl->m_RenderingExecutionTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Rendering Execution Task", ITask::Type::Recurrent, 2), [&]()
			{
				if (Get<HIDService>()->IsResizing())
					return true;

				auto l_tickStartTime = Get<Timer>()->GetCurrentTimeFromEpoch();

				Get<FrameManagementService>()->Update();

				auto l_tickEndTime = Get<Timer>()->GetCurrentTimeFromEpoch();

				m_pImpl->m_tickTime = float(l_tickEndTime - l_tickStartTime) / 1000.0f;

				return true;
			});
	}

	m_pImpl->m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "Engine setup finished.");

	return true;
}

#include "Engine_Internal.h"
#include "Common/LogService.h"
#include "Common/TaskScheduler.h"
#include "Services/EntityRegistry.h"
#include "Services/TransformService.h"
#include "Services/LightSimulationService.h"
#include "Services/CameraService.h"
#include "Services/SceneService.h"
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

using namespace Inno;

bool Engine::Initialize()
{
	SystemInit(HIDService);
	m_pImpl->m_WindowSystem->Initialize();

	SystemInit(EntityRegistry);
	SystemInit(TransformService);

	SystemInit(SceneService);
	SystemInit(PhysicsSimulationService);

	SystemInit(LightSimulationService);
	SystemInit(CameraService);

	if (m_pImpl->m_initConfig.engineMode == EngineMode::Sidecar)
	{
		SystemInit(EditorService);
	}

	if (!m_pImpl->m_initConfig.isHeadless) {
		Get<FrameManagementService>()->Initialize();
		SystemInit(TemplateAssetService);
		SystemInit(PerFrameDataService);
		SystemInit(LightDataService);
		SystemInit(DrawCallService);
		SystemInit(AnimationDrawCallService);
		SystemInit(BillboardDrawCallService);
		SystemInit(DebugDrawCallService);
		SystemInit(AnimationSimulationService);
		SystemInit(AnimationResourceService);

		ITask::Desc taskDesc("Default Rendering Client Initialization Task", ITask::Type::Once, 2);
		auto l_ExampleRenderingClientInitializationTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc, [=]() {
			if (m_pImpl->m_RenderingClient) {
				if (!m_pImpl->m_RenderingClient->Initialize())
				{
					Log(Error, "Rendering Client can't be initialized!");
					return false;
				}
			}

			if (!m_pImpl->m_initConfig.isOffscreen)
				SystemInit(GUIService);

			return true;
			});

		l_ExampleRenderingClientInitializationTask->Activate();
		l_ExampleRenderingClientInitializationTask->Wait();

		if (m_pImpl->m_RenderingExecutionTask)
		{
			Log(Verbose, "Activating rendering execution task...");
			m_pImpl->m_RenderingExecutionTask->Activate();
		}
		else
		{
			Log(Error, "m_RenderingExecutionTask is null!");
		}
	}

	if (m_pImpl->m_LogicClient && !m_pImpl->m_initConfig.isBakeMode) {
		m_pImpl->m_LogicClient->Initialize();
	}

	m_pImpl->m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "Engine has been initialized.");

	return true;
}

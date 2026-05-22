#pragma once
#include "Engine.h"
#include "Common/STL14.h"
#include "Common/SharedPtr.h"
#include "Common/Task.h"
#include "Common/FixedSizeString.h"
#include "Interface/IWindowService.h"

namespace Inno
{
	class EngineImpl
	{
	public:
		InitConfig m_initConfig;

		std::unique_ptr<IWindowService> m_WindowSystem;

		std::unique_ptr<IRenderingClient> m_RenderingClient;
		std::unique_ptr<ILogicClient> m_LogicClient;

		FixedSizeString<128> m_applicationName;

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::atomic<bool> m_isRendering = false;
		std::atomic<bool> m_allowRender = false;

		std::function<void()> f_SceneLoadingStartedCallback;
		std::function<void()> f_SceneLoadingFinishedCallback;

		SharedPtr<ITask> m_RenderingExecutionTask;

		float m_tickTime = 0;
	};
}

// Service-lifecycle macros — must expand inside an Engine:: member fn body
// (m_pImpl and Get<>() are both members).

#define SystemSetup( className ) \
if (!Get<##className>()->Setup(nullptr)) \
{ \
	return false; \
} \

#define SystemInit( className ) \
if (!Get<##className>()->Initialize()) \
{ \
	return false; \
} \

#define SystemUpdate( className ) \
if (!Get<##className>()->Update()) \
{ \
m_pImpl->m_ObjectStatus = ObjectStatus::Suspended; \
return false; \
}

#define SystemTerm( className ) \
if (!Get<##className>()->Terminate()) \
{ \
	return false; \
} \

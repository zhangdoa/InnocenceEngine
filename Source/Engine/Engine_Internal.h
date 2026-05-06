#pragma once
#include "Engine.h"
#include "Common/STL14.h"
#include "Common/Handle.h"
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

		Handle<ITask> m_RenderingExecutionTask;

		float m_tickTime = 0;
	};
}

// Sibling-TU service-lifecycle macros. Hoisted out of Engine.cpp because
// Engine_Setup.cpp / Engine_Initialize.cpp / Engine_Terminate.cpp all expand
// these — keeping them TU-local would force per-file redefinitions.
//
// `m_pImpl` is a member of `Engine`, so each macro must expand inside an
// `Engine::` member fn body. `Get<##className>()` is also a member-template
// call — the `##` token-paste form is preserved verbatim from the original.

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

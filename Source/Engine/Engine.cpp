#include "Engine_Internal.h"
#include "Common/Array.h"
#include "Common/LogService.h"

#if defined INNO_PLATFORM_WIN
#include "Platform/WinWindow/WinWindowService.h"
#endif
#if defined INNO_PLATFORM_MAC
#include "Platform/MacWindow/MacWindowService.h"
#endif
#if defined INNO_PLATFORM_LINUX
#include "Platform/LinuxWindow/LinuxWindowService.h"
#endif

#include "Platform/HeadlessWindow/HeadlessWindowService.h"

#include "Services/HIDService.h"
#include "Common/Timer.h"

namespace Inno
{
	Engine* g_Engine = nullptr;
}

using namespace Inno;

IWindowService* Engine::CreateWindowSystem(bool isHeadless)
{
	if (isHeadless) {
		return new HeadlessWindowService();
	}

#if defined INNO_PLATFORM_WIN
	return new WinWindowService();
#elif defined INNO_PLATFORM_MAC
	return new MacWindowService();
#elif defined INNO_PLATFORM_LINUX
	return new LinuxWindowService();
#else
	Log(Error, "No WindowSystem implementation available for this platform.");
	return nullptr;
#endif
}

void Engine::ResolveDependencies(const Inno::Array<std::type_index>& dependencies)
{
}

Engine::Engine()
{
	g_Engine = this;
	m_pImpl = new EngineImpl();
}

Engine::~Engine()
{
	delete m_pImpl;

	for (auto& pair : singletons_)
	{
		delete static_cast<char*>(pair.second);
	}

	g_Engine = nullptr;
}

bool Engine::ExecuteDefaultTask()
{
	Get<Timer>()->Tick();

	m_pImpl->m_WindowSystem->Update();
	SystemUpdate(HIDService);

	if (m_pImpl->m_WindowSystem->GetStatus() != ObjectStatus::Activated)
	{
		m_pImpl->m_ObjectStatus = ObjectStatus::Suspended;
		Log(Warning, "Engine is stand-by.");
		return false;
	}

	return true;
}

ObjectStatus Engine::GetStatus()
{
	return m_pImpl->m_ObjectStatus;
}

InitConfig Engine::getInitConfig()
{
	return m_pImpl->m_initConfig;
}

void Engine::setSerializeTestResult(int result)
{
	m_pImpl->m_initConfig.serializeTestResult = result;
}

IWindowService* Engine::getWindowService()
{
	return m_pImpl->m_WindowSystem.get();
}

float Engine::getTickTime()
{
	return m_pImpl->m_tickTime;
}

const FixedSizeString<128>& Engine::GetApplicationName()
{
	return m_pImpl->m_applicationName;
}

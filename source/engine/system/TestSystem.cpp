#include "TestSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoTestSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

bool InnoTestSystemNS::setup()
{
	return true;
}

bool InnoTestSystemNS::initialize()
{
	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TestSystem has been initialized.");
	return true;
}

bool InnoTestSystemNS::update()
{
	return true;
}

bool InnoTestSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TestSystem has been terminated.");

	return true;
}

INNO_SYSTEM_EXPORT bool InnoTestSystem::setup()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTestSystem::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTestSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTestSystem::terminate()
{
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus InnoTestSystem::getStatus()
{
	return InnoTestSystemNS::m_objectStatus;
}

bool InnoTestSystem::measure(const std::string& functorName, const std::function<void()>& functor)
{
	auto l_startTime = g_pCoreSystem->getTimeSystem()->getCurrentTimeFromEpoch();

	(functor)();

	auto l_endTime = g_pCoreSystem->getTimeSystem()->getCurrentTimeFromEpoch();

	auto l_duration = (float)(l_endTime - l_startTime);

	l_duration /= 1000000.0f;

	return true;
}
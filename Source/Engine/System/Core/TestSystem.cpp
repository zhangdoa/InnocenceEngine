#include "TestSystem.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoTestSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool InnoTestSystemNS::setup()
{
	InnoTestSystemNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoTestSystemNS::initialize()
{
	if (InnoTestSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoTestSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TestSystem has been initialized.");
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "TestSystem: Object is not created!");
		return false;
	}
}

bool InnoTestSystemNS::update()
{
	if (InnoTestSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		InnoTestSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoTestSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TestSystem has been terminated.");

	return true;
}

bool InnoTestSystem::setup()
{
	return InnoTestSystemNS::setup();
}

bool InnoTestSystem::initialize()
{
	return InnoTestSystemNS::initialize();
}

bool InnoTestSystem::update()
{
	return InnoTestSystemNS::update();
}

bool InnoTestSystem::terminate()
{
	return InnoTestSystemNS::terminate();
}

ObjectStatus InnoTestSystem::getStatus()
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
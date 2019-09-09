#include "TestSystem.h"
#include "InnoLogger.h"
#include "InnoTimer.h"

namespace InnoTestSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool InnoTestSystemNS::setup()
{
	InnoTestSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoTestSystemNS::initialize()
{
	if (InnoTestSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		InnoTestSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "TestSystem has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "TestSystem: Object is not created!");
		return false;
	}
}

bool InnoTestSystemNS::update()
{
	if (InnoTestSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		InnoTestSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoTestSystemNS::terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "TestSystem has been terminated.");

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
	return InnoTestSystemNS::m_ObjectStatus;
}

bool InnoTestSystem::measure(const std::string& functorName, const std::function<void()>& functor)
{
	auto l_startTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	(functor)();

	auto l_endTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_duration = (float)(l_endTime - l_startTime);

	l_duration /= 1000000.0f;

	return true;
}
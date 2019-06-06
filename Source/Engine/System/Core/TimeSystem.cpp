#include "TimeSystem.h"
#include "InnoTimer.h"

INNO_PRIVATE_SCOPE InnoTimeSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
};

bool InnoTimeSystem::setup()
{
	InnoTimeSystemNS::m_objectStatus = ObjectStatus::Created;
	return InnoTimer::Setup();
}

bool InnoTimeSystem::initialize()
{
	if (InnoTimeSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoTimeSystemNS::m_objectStatus = ObjectStatus::Activated;
		return InnoTimer::Initialize();
	}
	else
	{
		return false;
	}
}

bool InnoTimeSystem::update()
{
	if (InnoTimeSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		return InnoTimer::Tick();
	}
	else
	{
		InnoTimeSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoTimeSystem::terminate()
{
	InnoTimeSystemNS::m_objectStatus = ObjectStatus::Terminated;
	return InnoTimer::Terminate();
}

const TimeData InnoTimeSystem::getCurrentTime(unsigned int timezone_adjustment)
{
	auto l_resultRaw = InnoTimer::GetCurrentTime(timezone_adjustment);

	TimeData l_result;

	l_result.year = l_resultRaw.Year;
	l_result.month = l_resultRaw.Month;
	l_result.day = l_resultRaw.Day;
	l_result.hour = l_resultRaw.Hour;
	l_result.minute = l_resultRaw.Minute;
	l_result.second = l_resultRaw.Second;
	l_result.millisecond = l_resultRaw.Millisecond;

	return  l_result;
}

const long long InnoTimeSystem::getCurrentTimeFromEpoch()
{
	return InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
}

ObjectStatus InnoTimeSystem::getStatus()
{
	return InnoTimeSystemNS::m_objectStatus;
}
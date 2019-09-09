#include "TimeSystem.h"
#include "InnoTimer.h"

namespace InnoTimeSystemNS
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

const TimeData InnoTimeSystem::getCurrentTime(uint32_t timezone_adjustment)
{
	auto l_resultRaw = InnoTimer::GetCurrentTime(timezone_adjustment);

	TimeData l_result;

	l_result.Year = l_resultRaw.Year;
	l_result.Month = l_resultRaw.Month;
	l_result.Day = l_resultRaw.Day;
	l_result.Hour = l_resultRaw.Hour;
	l_result.Minute = l_resultRaw.Minute;
	l_result.Second = l_resultRaw.Second;
	l_result.Millisecond = l_resultRaw.Millisecond;

	return  l_result;
}

const int64_t InnoTimeSystem::getCurrentTimeFromEpoch()
{
	return InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
}

ObjectStatus InnoTimeSystem::getStatus()
{
	return InnoTimeSystemNS::m_objectStatus;
}
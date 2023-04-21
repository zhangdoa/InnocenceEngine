#include "TimeSystem.h"
#include "../Core/Timer.h"

using namespace Inno;
namespace Inno
{
	namespace TimeSystemNS
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

bool TimeSystem::Setup(ISystemConfig* systemConfig)
{
	TimeSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return Timer::Setup();
}

bool TimeSystem::Initialize()
{
	if (TimeSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		TimeSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		return Timer::Initialize();
	}
	else
	{
		return false;
	}
}

bool TimeSystem::Update()
{
	if (TimeSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return Timer::Tick();
	}
	else
	{
		TimeSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool TimeSystem::Terminate()
{
	TimeSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	return Timer::Terminate();
}

const TimeData TimeSystem::getCurrentTime(uint32_t timezone_adjustment)
{
	auto l_resultRaw = Timer::GetCurrentTime(timezone_adjustment);

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

const int64_t TimeSystem::getCurrentTimeFromEpoch()
{
	return Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
}

ObjectStatus TimeSystem::GetStatus()
{
	return TimeSystemNS::m_ObjectStatus;
}
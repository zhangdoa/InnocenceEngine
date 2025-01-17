#include "Timer.h"
#include <chrono>

using namespace Inno;

Timer::Timer()
{
	m_EngineStartTime = HRClock::now();
	m_FrameLength = (1.0 / 120.0) * 1000.0 * 1000.0;
}

void Timer::Tick()
{
	m_UnprocessedTime++;
	if (m_UnprocessedTime >= m_FrameLength)
	{
		m_UnprocessedTime -= m_FrameLength;
	}
}

uint64_t Timer::GetCurrentTimeFromEpoch(TimeUnit time_unit)
{
	auto l_CurrentTime = HRClock::now().time_since_epoch();
	uint64_t l_result;

	switch (time_unit)
	{
	case TimeUnit::Microsecond:
		l_result = std::chrono::duration_cast<std::chrono::microseconds>(l_CurrentTime).count();
		break;
	case TimeUnit::Millisecond:
		l_result = std::chrono::duration_cast<std::chrono::milliseconds>(l_CurrentTime).count();
		break;
	case TimeUnit::Second:
		l_result = std::chrono::duration_cast<std::chrono::seconds>(l_CurrentTime).count();
		break;
	case TimeUnit::Minute:
		l_result = std::chrono::duration_cast<std::chrono::minutes>(l_CurrentTime).count();
		break;
	case TimeUnit::Hour:
		l_result = std::chrono::duration_cast<std::chrono::hours>(l_CurrentTime).count();
		break;
	case TimeUnit::Day:
		l_result = std::chrono::duration_cast<Days>(l_CurrentTime).count();
		break;
	case TimeUnit::Month:
		l_result = std::chrono::duration_cast<Months>(l_CurrentTime).count();
		break;
	case TimeUnit::Year:
		l_result = std::chrono::duration_cast<Years>(l_CurrentTime).count();
		break;
	default:
		break;
	}

	return l_result;
}

Timestamp Timer::GetCurrentTime(uint32_t time_zone_adjustment)
{
	auto tp = std::chrono::system_clock::now().time_since_epoch();

	tp += std::chrono::hours(time_zone_adjustment);

	auto d = std::chrono::duration_cast<Days>(tp);
	tp -= d;
	auto h = std::chrono::duration_cast<std::chrono::hours>(tp);
	tp -= h;
	auto m = std::chrono::duration_cast<std::chrono::minutes>(tp);
	tp -= m;
	auto s = std::chrono::duration_cast<std::chrono::seconds>(tp);
	tp -= s;

	auto z = d.count();

	z += 719468;
	const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
	// [0, 146096]
	const unsigned doe = static_cast<unsigned>(z - era * 146097);
	// [0, 399]
	const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	const int32_t Year = static_cast<int32_t>(yoe) + era * 400;
	// [0, 365]
	const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	// [0, 11]
	const unsigned mp = (5 * doy + 2) / 153;
	// [1, 31]
	const unsigned Day = doy - (153 * mp + 2) / 5 + 1;
	// [1, 12]
	const unsigned Month = (mp < 10 ? mp + 3 : mp - 9);

	Timestamp l_Timestamp;

	l_Timestamp.Year = Year + (Month <= 2);
	l_Timestamp.Month = Month;
	l_Timestamp.Day = Day;
	l_Timestamp.Hour = h.count();
	l_Timestamp.Minute = m.count();
	l_Timestamp.Second = static_cast<uint32_t>(s.count());
	l_Timestamp.Millisecond = static_cast<uint32_t>(tp / std::chrono::milliseconds(1));
	l_Timestamp.Microsecond = static_cast<uint32_t>(tp / std::chrono::microseconds(1));

	return l_Timestamp;
}

bool Timer::Measure(const std::string& functorName, const std::function<void()>& functor)
{
	auto l_startTime = GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	(functor)();

	auto l_endTime = GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_duration = (float)(l_endTime - l_startTime);

	l_duration /= 1000000.0f;

	return true;
}